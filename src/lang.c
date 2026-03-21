#include "lang.h"
#include "handlers/handlers.h"
#include <string.h>
#include <ctype.h>

static const char *ext_c[] = {"c", "h", NULL};
static const char *ext_cpp[] = {"cpp", "cxx", "cc", "c++", "hpp", "hxx", "hh", "h++", "ipp", NULL};
static const char *ext_objc[] = {"m", NULL};
static const char *ext_objcpp[] = {"mm", NULL};
static const char *ext_java[] = {"java", NULL};
static const char *ext_js[] = {"js", "mjs", "cjs", "jsx", NULL};
static const char *ext_ts[] = {"ts", "mts", "cts", "tsx", NULL};
static const char *ext_csharp[] = {"cs", NULL};
static const char *ext_go[] = {"go", NULL};
static const char *ext_rust[] = {"rs", NULL};
static const char *ext_swift[] = {"swift", NULL};
static const char *ext_kotlin[] = {"kt", "kts", NULL};
static const char *ext_scala[] = {"scala", "sc", NULL};
static const char *ext_dart[] = {"dart", NULL};
static const char *ext_css[] = {"css", NULL};
static const char *ext_php[] = {"php", NULL};
static const char *ext_python[] = {"py", "pyi", "pyw", NULL};
static const char *ext_ruby[] = {"rb", "rake", NULL};
static const char *ext_shell[] = {"sh", "bash", "zsh", NULL};
static const char *ext_perl[] = {"pl", "pm", NULL};
static const char *ext_r[] = {"r", "R", NULL};
static const char *ext_yaml[] = {"yml", "yaml", NULL};
static const char *ext_toml[] = {"toml", NULL};
static const char *ext_lua[] = {"lua", NULL};
static const char *ext_none[] = {NULL};

static const char *fn_makefile[] = {"Makefile", "makefile", "GNUmakefile", NULL};
static const char *fn_ruby[] = {"Rakefile", "Gemfile", NULL};
static const char *fn_shell[] = {".bashrc", ".bash_profile", ".zshrc", ".profile", NULL};
static const char *fn_none[] = {NULL};

#define HASH_SQ_NO_ESCAPE (1u << 16)

static const dc_lang_t languages[] = {
    {"c", "C", ext_c, fn_none, dc_strip_clike, CLIKE_C, NULL},
    {"cpp", "C++", ext_cpp, fn_none, dc_strip_clike, CLIKE_CPP, NULL},
    {"objc", "Objective-C", ext_objc, fn_none, dc_strip_clike, CLIKE_C, NULL},
    {"objcpp", "Objective-C++", ext_objcpp, fn_none, dc_strip_clike, CLIKE_CPP, NULL},
    {"java", "Java", ext_java, fn_none, dc_strip_clike, CLIKE_JAVA, NULL},
    {"js", "JavaScript", ext_js, fn_none, dc_strip_clike, CLIKE_JS, NULL},
    {"ts", "TypeScript", ext_ts, fn_none, dc_strip_clike, CLIKE_TS, NULL},
    {"csharp", "C#", ext_csharp, fn_none, dc_strip_clike, CLIKE_CSHARP, NULL},
    {"go", "Go", ext_go, fn_none, dc_strip_clike, CLIKE_GO, NULL},
    {"rust", "Rust", ext_rust, fn_none, dc_strip_clike, CLIKE_RUST, NULL},
    {"swift", "Swift", ext_swift, fn_none, dc_strip_clike, CLIKE_SWIFT, NULL},
    {"kotlin", "Kotlin", ext_kotlin, fn_none, dc_strip_clike, CLIKE_KOTLIN, NULL},
    {"scala", "Scala", ext_scala, fn_none, dc_strip_clike, CLIKE_SCALA, NULL},
    {"dart", "Dart", ext_dart, fn_none, dc_strip_clike, CLIKE_DART, NULL},
    {"css", "CSS", ext_css, fn_none, dc_strip_clike, CLIKE_CSS, NULL},
    {"php", "PHP", ext_php, fn_none, dc_strip_clike, CLIKE_PHP,
     "Limited: does not handle mixed HTML/PHP or heredocs"},
    {"python", "Python", ext_python, fn_none, dc_strip_python, 0, NULL},
    {"ruby", "Ruby", ext_ruby, fn_ruby, dc_strip_hash, 0,
     "Limited: does not handle heredocs, regex literals, or %q{} quoting"},
    {"shell", "Shell", ext_shell, fn_shell, dc_strip_shell, 0,
     "Limited: does not handle heredocs"},
    {"make", "Makefile", ext_none, fn_makefile, dc_strip_hash, 0,
     "Limited: does not handle define/endef blocks"},
    {"perl", "Perl", ext_perl, fn_none, dc_strip_hash, 0,
     "Limited: does not handle heredocs, regex, or complex quoting"},
    {"r", "R", ext_r, fn_none, dc_strip_hash, 0, NULL},
    {"yaml", "YAML", ext_yaml, fn_none, dc_strip_hash, HASH_SQ_NO_ESCAPE, NULL},
    {"toml", "TOML", ext_toml, fn_none, dc_strip_hash, HASH_SQ_NO_ESCAPE, NULL},
    {"lua", "Lua", ext_lua, fn_none, dc_strip_lua, 0, NULL},
    {NULL, NULL, NULL, NULL, NULL, 0, NULL}  
};

 
static const dc_lang_t *lang_ptrs[sizeof(languages) / sizeof(languages[0])];

void dc_lang_init(void)
{
    for (size_t i = 0; languages[i].name != NULL; i++)
    {
        lang_ptrs[i] = &languages[i];
    }
     
    size_t count = 0;
    while (languages[count].name)
        count++;
    lang_ptrs[count] = NULL;
}

const dc_lang_t *const *dc_lang_all(void)
{
    return lang_ptrs;
}

const dc_lang_t *dc_lang_find(const char *name)
{
    if (!name)
        return NULL;
    for (size_t i = 0; languages[i].name; i++)
    {
        if (strcmp(languages[i].name, name) == 0)
            return &languages[i];
        const char *a = languages[i].display_name;
        const char *b = name;
        int match = 1;
        while (*a && *b)
        {
            if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            {
                match = 0;
                break;
            }
            a++;
            b++;
        }
        if (match && !*a && !*b)
            return &languages[i];
    }
    return NULL;
}

static const dc_lang_t *detect_by_filename(const char *filename)
{
    for (size_t i = 0; languages[i].name; i++)
    {
        if (!languages[i].filenames)
            continue;
        for (int j = 0; languages[i].filenames[j]; j++)
        {
            if (strcmp(filename, languages[i].filenames[j]) == 0)
                return &languages[i];
        }
    }
    return NULL;
}

static const dc_lang_t *detect_by_extension(const char *ext)
{
    if (!ext)
        return NULL;
    for (size_t i = 0; languages[i].name; i++)
    {
        if (!languages[i].extensions)
            continue;
        for (int j = 0; languages[i].extensions[j]; j++)
        {
            if (strcmp(ext, languages[i].extensions[j]) == 0)
                return &languages[i];
        }
    }
    return NULL;
}

const dc_lang_t *dc_lang_detect_shebang(const char *data, size_t len)
{
    if (len < 3 || data[0] != '#' || data[1] != '!')
        return NULL;

    size_t end = 2;
    while (end < len && end < 256 && data[end] != '\n')
        end++;

    const char *line = data + 2;
    size_t line_len = end - 2;

    struct
    {
        const char *pattern;
        const char *lang;
    } shebangs[] = {
        {"python", "python"},
        {"ruby", "ruby"},
        {"bash", "shell"},
        {"sh", "shell"},
        {"zsh", "shell"},
        {"perl", "perl"},
        {"lua", "lua"},
        {"node", "js"},
        {NULL, NULL}};

    for (int i = 0; shebangs[i].pattern; i++)
    {
        size_t plen = strlen(shebangs[i].pattern);
        for (size_t j = 0; j + plen <= line_len; j++)
        {
            if (memcmp(line + j, shebangs[i].pattern, plen) == 0)
            {
                if (j > 0 && line[j - 1] != '/' && line[j - 1] != ' ')
                    continue;
                return dc_lang_find(shebangs[i].lang);
            }
        }
    }

    return NULL;
}

static const char *get_ext(const char *path)
{
    const char *dot = NULL;
    const char *sep = path;
    for (const char *p = path; *p; p++)
    {
        if (*p == '/' || *p == '\\')
            sep = p + 1;
        if (*p == '.')
            dot = p;
    }
    if (!dot || dot < sep)
        return NULL;
    return dot + 1;
}

static const char *get_fname(const char *path)
{
    const char *name = path;
    for (const char *p = path; *p; p++)
    {
        if (*p == '/' || *p == '\\')
            name = p + 1;
    }
    return name;
}

const dc_lang_t *dc_lang_detect(const char *path, const char *force_lang)
{
    if (force_lang)
        return dc_lang_find(force_lang);

    const char *filename = get_fname(path);

    const dc_lang_t *lang = detect_by_filename(filename);
    if (lang)
        return lang;

    const char *ext = get_ext(path);
    lang = detect_by_extension(ext);
    if (lang)
        return lang;

    return NULL;
}

int dc_ext_in_list(const char *ext, const char *csv_list)
{
    if (!ext || !csv_list)
        return 0;
    size_t ext_len = strlen(ext);
    const char *p = csv_list;
    while (*p)
    {
        while (*p == ',' || *p == ' ')
            p++;
        if (!*p)
            break;
        const char *start = p;
        while (*p && *p != ',' && *p != ' ')
            p++;
        size_t entry_len = (size_t)(p - start);
        if (entry_len > 0 && start[0] == '.')
        {
            start++;
            entry_len--;
        }
        if (entry_len == ext_len && memcmp(start, ext, ext_len) == 0)
            return 1;
    }
    return 0;
}
