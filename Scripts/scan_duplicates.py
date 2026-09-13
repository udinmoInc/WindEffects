#!/usr/bin/env python3
"""
Scan C++ codebase for duplicate and near-duplicate code patterns.
"""
import os
import re
from pathlib import Path
from typing import List, Tuple, Dict
from collections import defaultdict
import difflib

def normalize_code(code: str) -> str:
    """Normalize code for comparison by removing whitespace and comments."""
    # Remove single-line comments
    code = re.sub(r'//.*$', '', code, flags=re.MULTILINE)
    # Remove multi-line comments
    code = re.sub(r'/\*.*?\*/', '', code, flags=re.DOTALL)
    # Normalize whitespace
    code = re.sub(r'\s+', ' ', code)
    code = code.strip()
    return code

def extract_functions(filepath: Path) -> List[Tuple[str, str, int, int]]:
    """Extract function bodies from a C++ file."""
    functions = []
    
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except Exception:
        return functions
    
    # Simple pattern matching for function definitions
    # This is a basic heuristic - a proper parser would be better
    patterns = [
        r'(\w+(?:<[^>]+>)?\s*\([^)]*\)\s*(?:const|override|final)?\s*\{)',
        r'(\w+\s+(?:\*|&)?\s*\w+\s*\([^)]*\)\s*(?:const|override|final)?\s*\{)',
    ]
    
    lines = content.split('\n')
    i = 0
    while i < len(lines):
        line = lines[i]
        
        for pattern in patterns:
            match = re.search(pattern, line)
            if match:
                func_start = i
                brace_count = 0
                in_function = False
                func_lines = []
                
                # Find the function body
                for j in range(i, len(lines)):
                    func_line = lines[j]
                    brace_count += func_line.count('{')
                    brace_count -= func_line.count('}')
                    
                    if not in_function and '{' in func_line:
                        in_function = True
                    
                    if in_function:
                        func_lines.append(func_line)
                    
                    if in_function and brace_count == 0:
                        func_end = j
                        func_body = '\n'.join(func_lines)
                        func_signature = match.group(1)
                        functions.append((func_signature, func_body, func_start + 1, func_end + 1))
                        i = j
                        break
                break
        i += 1
    
    return functions

def compute_similarity(code1: str, code2: str) -> float:
    """Compute similarity ratio between two code blocks."""
    norm1 = normalize_code(code1)
    norm2 = normalize_code(code2)
    
    if not norm1 or not norm2:
        return 0.0
    
    return difflib.SequenceMatcher(None, norm1, norm2).ratio()

def find_duplicates(root_dir: Path, threshold: float = 0.85) -> Dict[str, List[Tuple]]:
    """Find duplicate code patterns across the codebase."""
    all_functions = []
    
    cpp_extensions = ['.cpp', '.h', '.hpp', '.cc', '.cxx']
    
    # Collect all functions
    for filepath in root_dir.rglob('*'):
        if filepath.suffix in cpp_extensions:
            if 'ThirdParty' in str(filepath):
                continue
            
            functions = extract_functions(filepath)
            for sig, body, start, end in functions:
                all_functions.append((str(filepath), sig, body, start, end))
    
    # Compare functions for duplicates
    duplicates = defaultdict(list)
    
    for i in range(len(all_functions)):
        for j in range(i + 1, len(all_functions)):
            file1, sig1, body1, start1, end1 = all_functions[i]
            file2, sig2, body2, start2, end2 = all_functions[j]
            
            # Skip if same file
            if file1 == file2:
                continue
            
            # Skip if signatures are identical (likely same function)
            if sig1 == sig2:
                continue
            
            similarity = compute_similarity(body1, body2)
            
            if similarity >= threshold:
                key = f"{file1}::{sig1}"
                duplicates[key].append((file2, sig2, similarity, start1, end1, start2, end2))
    
    return duplicates

def main():
    """Main entry point."""
    script_dir = Path(__file__).parent
    root_dir = script_dir.parent / 'Engine' / 'Source'
    
    if not root_dir.exists():
        print(f"Error: Directory not found: {root_dir}")
        return
    
    print(f"Scanning C++ files for duplicates in: {root_dir}")
    print("=" * 80)
    
    duplicates = find_duplicates(root_dir, threshold=0.85)
    
    total_groups = len(duplicates)
    total_duplicates = sum(len(v) for v in duplicates.values())
    
    print(f"\nFound {total_duplicates} potential duplicates in {total_groups} groups\n")
    
    for key, dups in sorted(duplicates.items()):
        print(f"\n{key}")
        print("-" * 80)
        for file2, sig2, similarity, start1, end1, start2, end2 in dups:
            rel_path1 = Path(key.split('::')[0]).relative_to(root_dir.parent.parent)
            rel_path2 = Path(file2).relative_to(root_dir.parent.parent)
            print(f"  Similar to: {rel_path2}::{sig2}")
            print(f"  Similarity: {similarity:.2%}")
            print(f"  Locations: Lines {start1}-{end1} vs {start2}-{end2}")
            print()
    
    print("=" * 80)
    print(f"Total: {total_duplicates} potential duplicates in {total_groups} groups")

if __name__ == '__main__':
    main()
