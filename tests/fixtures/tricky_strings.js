// Regex vs division
let a = 10 / 2; // division
let b = /regex/g; // regex
let c = (x) / y; // division (after closing paren)
let d = [1] / 2; // division (after closing bracket... tricky)

// Template literal nesting
let e = `outer ${`inner`} end`;
let f = `outer ${ x + `nested ${y}` } end`;

// Regex with special chars
let g = /[/]/;
let h = /\//;
let i = /[^/]+/;

// String edge cases
let j = "hello 'world'";
let k = 'hello "world"';

// Comment-like in template
let l = `// not a comment /* also not */`;

// Regex after keywords
if (/test/.test(x)) {}
return /pattern/;
