#!/bin/bash
# Copyright 2025 Felix P. A. Gillberg HolyBooter
# SPDX-License-Identifier: GPL-2.0


# Remove license headers from C and Assembly files
# Usage: ./remove_headers.sh

echo "Removing license headers from C and Assembly files..."

find . -type f \( -name "*.*" -o -name "*.diff" \) | while read file; do
    echo "Processing: $file"

    # Create temporary file
    temp_file=$(mktemp)

    # Use awk to remove comment blocks at the beginning of files
    awk '
    BEGIN { in_header = 1; in_comment = 0 }

    # Track multi-line comments /* */
    /\/\*/ {
        if (in_header) in_comment = 1
    }
    /\*\// {
        if (in_header && in_comment) {
            in_comment = 0
            next
        }
    }

    # If we are in header and encounter non-comment, stop header processing
    (in_header && !in_comment) && !/^\/\// && !/^\/\*/ && !/^\s*\*/ && !/^\s*$/ {
        in_header = 0
    }

    # Print lines that are not part of the header
    !in_header && !in_comment { print }

    # Handle single-line comments in header
    in_header && /^\/\// { next }
    in_header && /^\s*\/\// { next }

    ' "$file" > "$temp_file"

    # Only replace original if temp file is not empty
    if [ -s "$temp_file" ]; then
        mv "$temp_file" "$file"
    else
        echo "Warning: $file would be empty, skipping"
        rm "$temp_file"
    fi
done

echo "Header removal complete."
