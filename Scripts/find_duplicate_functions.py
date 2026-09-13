#!/usr/bin/env python3
"""
Find duplicate function implementations across C++ files.
"""
import os
import re
from pathlib import Path
from collections import defaultdict

def extract_function_signatures(filepath):
    """Extract function signatures from a C++ file."""
    signatures = []
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except Exception:
        return signatures
    
    # Match function definitions
    pattern = r'(\w+(?:<[^>]+>)?\s*\([^)]*\)\s*(?:const|override|final)?\s*\{)'
    matches = re.finditer(pattern, content)
    
    for match in matches:
        signature = match.group(1)
        signatures.append(signature)
    
    return signatures

def find_duplicate_signatures(root_dir):
    """Find duplicate function signatures across the codebase."""
    signature_map = defaultdict(list)
    
    cpp_extensions = ['.cpp', '.h', '.hpp']
    
    for filepath in root_dir.rglob('*'):
        if filepath.suffix in cpp_extensions:
            if 'ThirdParty' in str(filepath):
                continue
            
            signatures = extract_function_signatures(filepath)
            for sig in signatures:
                signature_map[sig].append(str(filepath))
    
    # Find signatures that appear in multiple files
    duplicates = {sig: files for sig, files in signature_map.items() if len(files) > 1}
    return duplicates

def main():
    script_dir = Path(__file__).parent
    root_dir = script_dir.parent / 'Engine' / 'Source'
    
    if not root_dir.exists():
        print(f"Error: Directory not found: {root_dir}")
        return
    
    print(f"Scanning for duplicate function signatures in: {root_dir}")
    print("=" * 80)
    
    duplicates = find_duplicate_signatures(root_dir)
    
    total_duplicates = len(duplicates)
    print(f"\nFound {total_duplicates} duplicate function signatures\n")
    
    for sig, files in sorted(duplicates.items()):
        print(f"\nSignature: {sig}")
        print("-" * 80)
        for file in files:
            rel_path = Path(file).relative_to(root_dir.parent.parent)
            print(f"  {rel_path}")
    
    print("=" * 80)
    print(f"Total: {total_duplicates} duplicate signatures")

if __name__ == '__main__':
    main()
