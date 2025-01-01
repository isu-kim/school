# SPDX-License-Identifier: Apache-2.0

.PHONY: build
build: 
	cd bpf && make generate && cd ..
	go mod tidy
	go build
