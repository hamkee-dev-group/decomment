#!/usr/bin/env python3

import sys  

def hello():
    """This is a docstring - NOT a comment"""
    x = "hello # not a comment"
    y = 'world # also not a comment'
    z = '''triple
    single # not a comment
    '''
    
    print(x + y)  


hello()
