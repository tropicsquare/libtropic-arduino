# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- API: `eccKeyGenerate`, `eccKeyStore`, `eccKeyRead`, `eccKeyErase`, `ecdsaSign`, `eddsaSign`, `rMemWrite`, `rMemRead`, `rMemErase`, `macAndDestroy`
- Examples: ECDSA_P256, EdDSA_Ed25519, R-Memory, MAC_and_destroy

### Changed
- Use camelCase for method parameters.

### Fixed

### Removed

## [0.1.0]

### Added
- Support in PlatformIO.
- Minimal C++ wrapper around Libtropic.
- `hello_world` example
