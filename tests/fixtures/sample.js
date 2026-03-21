// Line comment
/* Block comment */
const x = 42; // inline

const str = "hello // not a comment";
const str2 = 'world /* not a comment */';
const tmpl = `template // ${x + /* not stripped */ 1} end`;
const re = /pattern/gi;
const y = a / b; // division, not regex
const re2 = /[/]/; // regex with slash in class

/* Multi-line
   block
   comment */
console.log(str);
