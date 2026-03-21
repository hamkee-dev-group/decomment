#!/usr/bin/env python3
# This is a comment
import sys  # inline comment

def hello():
    """This is a docstring - NOT a comment"""
    x = "hello # not a comment"
    y = 'world # also not a comment'
    z = '''triple
    single # not a comment
    '''
    # This is a comment
    print(x + y)  # another comment

# Final comment
hello()
