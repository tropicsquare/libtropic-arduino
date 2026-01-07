# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- FAQ: section "`Tropic01.begin()` fails".

### Changed
- Examples: Call `SPI.begin`, because `SPI.begin` and `SPI.end` are no longer called by the Arduino HAL (see [here](https://github.com/tropicsquare/libtropic/pull/401) for more information). It is now expected that users initialize SPI in their code themselves.
- Examples: Enhanced error logging.
- Examples: Enhanced cleanup.

### Removed
- `rngSeed` parameter from `Tropic01()` constructor - it is now user's responsibility to initialize the RNG if needed.

## [0.2.0]

### Added
- API: `eccKeyGenerate`, `eccKeyStore`, `eccKeyRead`, `eccKeyErase`, `ecdsaSign`, `eddsaSign`, `rMemWrite`, `rMemRead`, `rMemErase`, `macAndDestroy`.
- Examples: ECDSA_P256, EdDSA_Ed25519, R-Memory, MAC_and_destroy.

### Changed
- Use camelCase for method parameters.

## [0.1.0]

### Added
- Support in PlatformIO.
- Minimal C++ wrapper around Libtropic.
- `hello_world` example
