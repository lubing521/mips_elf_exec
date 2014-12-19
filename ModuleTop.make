#!/bin/bash

unexport DYNLD_TARGET_ROOT
unexport DYNLD_TARGET
export DYNLD_TARGET_ROOT := $(shell pwd)

export DYNLD_TARGET :=

