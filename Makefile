# Build tools
#
# Targets (see each target for more information):
#   build:	builds binaries for specified architecture
#   build-shell:	enter the builder docker image
#   clean:	removes build artifacts and images
#   package: create debian package
#   push:	pushes debian package to repo
#
#   all-build:	builds binaries for all target architectures
#

###
### Customize  these variables
###

# The binary to build (just the basename).
NAME := vosion

# Where to pull the docker image.
REGISTRY ?=

# Which architecture to build - see $(ALL_ARCH) for options.
ARCH ?= armhf

# This version-strategy uses git tags to set the version string
VERSION ?= 0.1.0

###
### These variables should not need tweaking.
###


DOCKER_USER := $(shell if [[ "$$OSTYPE" != "darwin"* ]]; then USER_ARG="--user=`id -u`"; fi; echo "$$USER_ARG")

ALL_ARCH := amd64 i386 armhf aarch64

BASE_BUILD_IMAGE ?= multiarch/ubuntu-core:$(ARCH)-xenial
BUILD_IMAGE ?= $(NAME)-builder-$(ARCH)

ifeq ($(ARCH),aarch64)
  ARCH_ALTERNATE=arm64
else
  ARCH_ALTERNATE=$(ARCH)
endif

# Default target
all: build

# Builds the binary in a Docker container and copy to volume mount
build-%:
	@$(MAKE) --no-print-directory ARCH=$* build

image-clean-%:
	@$(MAKE) --no-print-directory ARCH=$* image-clean

# Builds all the binaries in a Docker container and copies to volume mount
all-build: $(addprefix build-, $(ALL_ARCH))

all-clean: $(addprefix image-clean-, $(ALL_ARCH))

build: build-dirs .image-$(BUILD_IMAGE)
	@echo "Running $@"
	@echo "    With Docker args: $(RUN_PROXY) $(DOCKER_RUN_OPTS)"
	@echo $(DOCKER_USER)
	@docker run                                                            \
	    -t                                                                 \
	    --rm                                                               \
	    $(DOCKER_USER)                                                     \
	    $(DOCKER_RUN_OPTS)                                                 \
			-v $$(pwd):/$(NAME)/                                               \
			-w /$(NAME)/build/$(ARCH)                                          \
	    -e NAME=$(NAME)                                                    \
			-e VERSION=$(VERSION)                                              \
	    $(BUILD_IMAGE)                                                     \
			/bin/sh -c "                                                       \
					cmake -DCMAKE_INSTALL_PREFIX=dist -DCMAKE_BUILD_TYPE=release ../../src/ && make install \
						     "

.image-$(BUILD_IMAGE): Dockerfile.build
	@echo "Building $@"
	@echo "    With Docker args: $(BUILD_PROXY) $(DOCKER_BUILD_OPTS)"
	@sed \
	    -e 's|ARG_FROM|$(BASE_BUILD_IMAGE)|g' \
	    -e 's|ARG_ARCH|$(ARCH)|g' \
	    Dockerfile.build > .dockerfile-$(BUILD_IMAGE)
	@docker build \
	    $(DOCKER_BUILD_OPTS) \
	    -t $(BUILD_IMAGE) \
	    -f .dockerfile-$(BUILD_IMAGE) .
	@docker images -q > $@

build-shell: .image-$(BUILD_IMAGE)
	@echo "Entering build shell..."
	@echo "    With Docker args: $(RUN_PROXY) $(DOCKER_RUN_OPTS)"
	@echo $(DOCKER_USER)
	@docker run                                                            \
	    -it                                                                \
	    --rm                                                               \
	    --cap-add SYS_PTRACE                                               \
		--network="host" 													\
	    $(DOCKER_RUN_OPTS)                                                 \
		-v $$(pwd):/$(NAME)/                                               \
		-w /$(NAME)/build/$(ARCH)                                          \
	    -e NAME=$(NAME)                                                    \
		-e VERSION=$(VERSION)                                              \
	    $(BUILD_IMAGE)                                                     \
	    /bin/bash

build-dirs:
	@mkdir -p build/$(ARCH)

clean: build-clean image-clean

image-clean:
	@if [ $(shell docker ps -a | grep $(BUILD_IMAGE) | wc -l) != 0 ]; then \
		docker ps -a | grep $(BUILD_IMAGE) | awk '{print $$1 }' | xargs docker rm -f; \
	fi
	@if [ $(shell docker images | grep $(BUILD_IMAGE) | wc -l) != 0 ]; then \
		docker images | grep $(BUILD_IMAGE) | awk '{print $$3}' | xargs docker rmi -f || true; \
	fi
	rm -rf .image-* .dockerfile-* .push-*

build-clean:
	@rm -rf build/$(ARCH)
	@echo "Build folder build/$(ARCH) cleaned"
