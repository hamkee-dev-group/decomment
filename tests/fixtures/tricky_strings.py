# String with hash
a = "hello # world"
b = 'hello # world'

# Triple-quoted with embedded quotes
c = """he said "hello" and 'bye'"""
d = '''she said "hello" and 'bye' '''

# Raw strings
e = r"hello\nworld # not a comment"
f = r'hello\nworld # not a comment'

# String with triple-like prefix
g = "" # empty string, this IS a comment
h = '' # also a comment

# Nested quotes in triple
i = """triple with "double" and 'single'"""  # comment
j = '''triple with "double" and 'single' '''  # comment

# Escaped hash... not a thing in Python, # is always a comment outside strings
