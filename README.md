# mjson - a JSON parser + emitter + JSON-RPC engine

[![Build Status](https://travis-ci.org/cesanta/mjson.svg?branch=master)](https://travis-ci.org/cesanta/mjson)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)

# Features

- Tiny footprint, single-file ISO C / ISO C++ library
- State machine parser, no allocations, no recursion
- High level API - fetch from JSON directly into C/C++ by
    [jsonpath](https://github.com/json-path/JsonPath)
- Low level SAX API
- Flexible JSON generation API - print to buffer, file, socket, etc
