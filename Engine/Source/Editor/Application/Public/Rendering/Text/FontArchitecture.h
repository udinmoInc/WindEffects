#pragma once

/**
 * @file FontArchitecture.h
 * @brief Modern MSDF Font Rendering System Architecture
 * 
 * This document describes the modular, cross-platform font rendering architecture.
 * Each module has a single responsibility and communicates through well-defined interfaces.
 * 
 * Architecture Principles:
 * - Single Responsibility: Each module has one clear purpose
 * - Dependency Injection: No global state or singletons
 * - Backend Independence: Abstract rendering from graphics API
 * - Data-Driven: Configuration through structured data
 * - Error-First: Comprehensive validation and error handling
 * - Performance-Oriented: Minimal allocations, efficient caching
 * 
 * Module Dependencies:
 * 
 * ┌─────────────────────────────────────────────────────────────────┐
 * │                        Application Layer                          │
 * └─────────────────────────────────────────────────────────────────┘
 *                                    │
 *                                    ▼
 * ┌─────────────────────────────────────────────────────────────────┐
 * │                      FontSystem (Facade)                          │
 * │  - Coordinates all modules                                        │
 * │  - Provides high-level text rendering API                        │
 * └─────────────────────────────────────────────────────────────────┘
 *                                    │
 *        ┌───────────┬───────────┬───────────┼───────────┬───────────┐
 *        ▼           ▼           ▼           ▼           ▼           ▼
 * ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐
 * │  Font    │ │ Unicode  │ │  Glyph   │ │  Atlas   │ │  Text    │ │  GPU     │
 * │  Loader  │ │ Decoder  │ │ Manager  │ │ Generator│ │  Shaper  │ │  Batch   │
 * └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘
 *        │           │           │           │           │           │
 *        ▼           ▼           ▼           ▼           ▼           ▼
 * ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐
 * │ FreeType │ │ UTF-8/16 │ │ HashMap  │ │ msdf-    │ │ HarfBuzz │ │ Vertex   │
 * │          │ │ /32      │ │ Storage  │ │ atlas-   │ │ + Fallback│ │ Buffer   │
 * │          │ │          │ │          │ │ gen      │ │          │ │ Manager  │
 * └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘
 *                                                                              │
 *                                                                              ▼
 *                                                                    ┌──────────────────┐
 *                                                                    │  Rendering       │
 *                                                                    │  Backend         │
 *                                                                    │  Abstraction     │
 *                                                                    └──────────────────┘
 *                                                                              │
 *                                                        ┌─────────────────────┼─────────────────────┐
 *                                                        ▼                     ▼                     ▼
 *                                                 ┌──────────┐          ┌──────────┐          ┌──────────┐
 * │  Vulkan   │          │ DirectX  │          │  Metal   │          │  OpenGL  │
 * └──────────┘          └──────────┘          └──────────┘          └──────────┘
 * 
 * Additional Modules:
 * - TextLayout: Word wrapping, alignment, multiline support
 * - AssetManager: Hot-reloading, async loading, caching
 * - Diagnostics: Validation, error reporting, debugging tools
 * - FallbackSystem: Comprehensive font fallback chain
 * - ShaderManager: MSDF shader compilation and management
 */

namespace we::UI::Text {

// ============================================================================
// Module 1: Font Loader
// ============================================================================
/**
 * Responsibility: Load font files (TTF, OTF) using FreeType
 * Dependencies: None (uses FreeType library)
 * Interfaces: IFontLoader
 * 
 * Features:
 * - Load TTF/OTF font files
 * - Extract font metrics (ascender, descender, line height)
 * - Support for font variations (weight, style)
 * - Font file validation
 * - Error reporting with detailed messages
 */

// ============================================================================
// Module 2: Unicode Decoder
// ============================================================================
/**
 * Responsibility: Decode UTF-8, UTF-16, UTF-32 to Unicode codepoints
 * Dependencies: None
 * Interfaces: IUnicodeDecoder
 * 
 * Features:
 * - UTF-8 decoding (variable-length 1-4 bytes)
 * - UTF-16 decoding (variable-length 1-2 code units)
 * - UTF-32 decoding (fixed-length)
 * - Validation of encoded sequences
 * - Replacement character (U+FFFD) for invalid sequences
 * - Bidirectional text detection
 */

// ============================================================================
// Module 3: Glyph Manager
// ============================================================================
/**
 * Responsibility: Store and retrieve glyph data by Unicode codepoint
 * Dependencies: Font Loader, Unicode Decoder
 * Interfaces: IGlyphManager
 * 
 * Features:
 * - Hash map storage keyed by Unicode codepoint
 * - Efficient glyph lookup (O(1) average)
 * - Glyph metrics (advance, bearing, size)
 * - Glyph cache management
 * - Missing glyph detection
 * - Statistics (glyph count, cache hit rate)
 */

// ============================================================================
// Module 4: Atlas Generator
// ============================================================================
/**
 * Responsibility: Generate MSDF font atlases using msdf-atlas-gen
 * Dependencies: Font Loader, Glyph Manager
 * Interfaces: IAtlasGenerator
 * 
 * Features:
 * - MSDF (Multi-channel Signed Distance Field) generation
 * - Configurable pixel range for sharp edges
 * - Atlas packing (power-of-two, tight packing)
 * - Support for pre-generated MSDF assets
 * - RGBA8 UNORM texture format
 * - Atlas dirty tracking for GPU uploads
 * - Multiple atlas support (texture arrays)
 */

// ============================================================================
// Module 5: Text Shaper
// ============================================================================
/**
 * Responsibility: Shape text with kerning, ligatures, and bidi support
 * Dependencies: Font Loader, Glyph Manager, Unicode Decoder
 * Interfaces: ITextShaper
 * 
 * Features:
 * - Kerning (pairwise glyph adjustments)
 * - Ligatures (glyph combinations)
 * - Bidirectional text (RTL/LTR)
 * - Script detection (Latin, Arabic, CJK, etc.)
 * - HarfBuzz integration (optional, fallback to basic shaping)
 * - Glyph substitution and positioning
 */

// ============================================================================
// Module 6: Text Layout
// ============================================================================
/**
 * Responsibility: Layout text with wrapping, alignment, and multiline support
 * Dependencies: Text Shaper, Glyph Manager
 * Interfaces: ITextLayout
 * 
 * Features:
 * - Word wrapping at character or word boundaries
 * - Text alignment (left, center, right, justified)
 * - Multiline text support
 * - Line height control
 * - Baseline alignment
 * - DPI scaling
 * - Measuring text dimensions
 */

// ============================================================================
// Module 7: GPU Batcher
// ============================================================================
/**
 * Responsibility: Batch glyphs for efficient GPU rendering
 * Dependencies: Atlas Generator, Text Layout
 * Interfaces: IGpuBatcher
 * 
 * Features:
 * - Persistent mapped vertex buffers
 * - Batch glyphs by atlas texture
 * - Minimal draw calls
 * - Vertex format (position, UV, color, etc.)
 * - Index buffer for quad rendering
 * - Dynamic buffer management
 * - Performance statistics (draw calls, vertex count)
 */

// ============================================================================
// Module 8: Shader Manager
// ============================================================================
/**
 * Responsibility: Manage MSDF shader compilation and binding
 * Dependencies: Backend Abstraction
 * Interfaces: IShaderManager
 * 
 * Features:
 * - MSDF shader with median distance evaluation
 * - Derivative-based anti-aliasing (fwidth)
 * - Linear texture sampling
 * - Pixel range calculation
 * - Gamma correction
 * - Shader permutation management
 * - Backend-specific shader compilation
 */

// ============================================================================
// Module 9: Asset Manager
// ============================================================================
/**
 * Responsibility: Manage font assets with hot-reloading and async loading
 * Dependencies: Font Loader, Atlas Generator
 * Interfaces: IAssetManager
 * 
 * Features:
 * - Hot-reloading of font files
 * - Asynchronous asset loading
 * - Asset caching
 * - Asset versioning
 * - Dependency tracking
 * - Memory management
 */

// ============================================================================
// Module 10: Diagnostics
// ============================================================================
/**
 * Responsibility: Provide validation, error reporting, and debugging tools
 * Dependencies: All modules
 * Interfaces: IDiagnostics
 * 
 * Features:
 * - Atlas preview visualization
 * - Loaded fonts list
 * - Glyph count statistics
 * - Missing glyph reports
 * - Texture format validation
 * - Cache statistics
 * - Draw call tracking
 * - GPU memory usage
 * - Validation logs
 * - Performance profiling
 */

// ============================================================================
// Module 11: Fallback System
// ============================================================================
/**
 * Responsibility: Provide comprehensive font fallback for missing glyphs
 * Dependencies: Font Loader, Glyph Manager
 * Interfaces: IFallbackSystem
 * 
 * Features:
 * - Fallback font chain (multiple fallback fonts)
 * - Script-based fallback (e.g., Arabic font for Arabic text)
 * - Automatic fallback selection
 * - Fallback glyph caching
 * - Missing glyph reporting
 * - Fallback performance tracking
 */

// ============================================================================
// Module 12: Backend Abstraction
// ============================================================================
/**
 * Responsibility: Provide graphics API-independent rendering interface
 * Dependencies: GPU Batcher, Shader Manager
 * Interfaces: IRenderingBackend
 * 
 * Features:
 * - Vulkan backend
 * - DirectX 12 backend
 * - Metal backend
 * - OpenGL backend
 * - Resource management (textures, buffers, samplers)
 * - Descriptor set management
 * - Command buffer recording
 * - Synchronization and barriers
 */

// ============================================================================
// Module 13: Font System (Facade)
// ============================================================================
/**
 * Responsibility: Coordinate all modules and provide high-level API
 * Dependencies: All modules
 * Interfaces: FontSystem
 * 
 * Features:
 * - Simple initialization and shutdown
 * - High-level text rendering API
 * - Module coordination
 * - Error handling and validation
 * - Configuration management
 * - Performance optimization
 */

} // namespace we::UI::Text
