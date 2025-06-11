const char *c_preproc_words[] = {
    "#define", "#ifdef", "#undef", "#include", "#ifndef", "#elif", "#endif", "#if", "#error", "#pragma",
    NULL};

const char *c_types[] = {
    "void", "int", "unsigned", "char", "long", "size_t", "bool", "float", "double",
    NULL};

const char *c_keywords[] = {
    "static", "const", "NULL", "sizeof", "malloc", "free", "typedef", "register", "struct", "union",
    "enum", "extern",
    NULL};

const char *c_flowcontrol[] = {
    "while", "switch", "case", "continue", "do", "break", "return", "if", "else", "for", "goto",
    NULL};

const char *c_comment_keywords[] = {
    "TODO:", "WARNING:", "NOTICE:", "@param", "@return", "@brief", "@author",
    NULL};

const char c_word_sepprators[] = {
    ' ', '(', ')', ',', '{', '}', '*', '&', ';',
    '\0'};

const char c_string_sep[] = {
    '"', '<', '>', '\'',
    '\0'};

const char c_comment_word_sep[] = {
    ' ',
    '\0'};

bool CReadWordFromLine(
    const char *sepprators,
    char *line, size_t linelen,   // line we are reading
    size_t *wstart, size_t *wend, // output word locations
    size_t *pos                   // where in the line we are (for itteration)
)
{
  *wstart = 0, *wend = 0;

  if (*pos >= linelen || line[*pos] == '\0')
    return false;

  while (isspace(line[*pos]) && line[*pos] != '\0')
    *pos += 1; // skips whitespace

  if (*pos >= linelen || line[*pos] == '\0')
    return false;

  size_t start = *pos;

  while (line[*pos] != '\0')
  {
    char ch = sepprators[0];
    bool hitsep = false;

    for (size_t i = 0; ch != '\0'; i++)
    {
      ch = sepprators[i];
      if (line[*pos] == ch)
      {
        goto SEARCH_DONE;
      }
    }

    *pos += 1;

    if (*pos >= linelen || line[*pos] == '\0')
      return false;
  }

SEARCH_DONE:
  *wstart = start;
  *wend = *pos;

  if (*pos >= linelen || line[*pos] == '\0')
    return false;

  if (line[*pos] != '\0') // skip what we hit
    *pos += 1;

  return true;
}

void DoCSyntaxHighlighing()
{
  // return;
  bool in_multiline_comment = false;

  // if syntax highlighing is too taxing, perhaps only re-computing hl for visable lines is in order
  // for (size_t i = g_topline; i < (g_screen_line_count + g_topline); i++)
  for (size_t i = 0; i < g_lines_count; i++)
  {

    struct Line l = g_lines[i];

    if (!l.base || !l.style || l.len == 0)
      continue; // this line is empty

    size_t pos = 0;
    char last = ' ';
    memset(l.style, COLOR_DEFAULT, l.len);
    size_t wstart = 0, wend = 0;
    // handles keywords and lang words
    while (CReadWordFromLine(c_word_sepprators, l.base, l.len, &wstart, &wend, &pos))
    {
      size_t wordlen = wend - wstart;
      const char *kw = NULL;
      int kw_idx = 0;
      for (;;)
      {
        kw = c_keywords[kw_idx];
        kw_idx += 1;

        if (!kw)
          break;

        size_t kwlen = strlen(kw);

        if (kwlen != wordlen)
          continue;

        if (memcmp(kw, l.base + wstart, kwlen) == 0)
        {
          memset(l.style + wstart, COLOR_KEYWORD, kwlen);
          goto WORD_DONE;
        }
      }

      kw = NULL;
      kw_idx = 0;
      for (;;)
      {
        kw = c_types[kw_idx];
        kw_idx += 1;

        if (!kw)
          break;

        size_t kwlen = strlen(kw);

        if (kwlen != wordlen)
          continue;

        if (memcmp(kw, l.base + wstart, kwlen) == 0)
        {
          memset(l.style + wstart, COLOR_TYPE, kwlen);
          goto WORD_DONE;
        }
      }

      kw = NULL;
      kw_idx = 0;
      for (;;)
      {
        kw = c_preproc_words[kw_idx];
        kw_idx += 1;

        if (!kw)
          break;

        size_t kwlen = strlen(kw);

        if (kwlen != wordlen)
          continue;

        if (memcmp(kw, l.base + wstart, kwlen) == 0)
        {
          memset(l.style + wstart, COLOR_PREPROC, kwlen);
          goto WORD_DONE;
        }
      }

      kw = NULL;
      kw_idx = 0;
      for (;;)
      {
        kw = c_flowcontrol[kw_idx];
        kw_idx += 1;

        if (!kw)
          break;

        size_t kwlen = strlen(kw);

        if (kwlen != wordlen)
          continue;

        if (memcmp(kw, l.base + wstart, kwlen) == 0)
        {
          memset(l.style + wstart, COLOR_FLOWCTRL, kwlen);
          goto WORD_DONE;
        }
      }

    WORD_DONE:;
    }

    pos = wstart = wend = 0;

    // handles string / char lits / imports
    while (CReadWordFromLine(c_string_sep, l.base, l.len, &wstart, &wend, &pos))
    {
      // Ensure we do not access out-of-bounds memory
      if (wstart == 0 || (wstart + (wend - wstart)) > l.len)
        continue;

      size_t len = (wend - wstart) + 2;
      const char *wrd = l.base + wstart - 1;
      char cs = '\0';

      int cs_idx = 0;
      // printf("%.*s\n", len, wrd);
      for (;;)
      {
        cs = c_string_sep[cs_idx];

        cs_idx += 1;

        if (!cs)
          break;

        // Check bounds before dereferencing
        if ((wstart - 1) < l.len && (wstart - 1) < l.cap && (wstart + (wend - wstart)) < l.len)
        {
          if (cs != '<' && cs != '>' && *wrd == cs && *(wrd + len - 1) == cs)
          {
            memset(l.style + wstart - 1, COLOR_STRCHAR, len);
            goto WORD_DONE_;
          }
          else if (*wrd == '<' && *(wrd + len - 1) == '>')
          {
            memset(l.style + wstart - 1, COLOR_STRCHAR, len);
            goto WORD_DONE_;
          }
        }
      }

    WORD_DONE_:
    }

    last = ' ';
    // single line // comments
    for (size_t j = 0; j < l.len; j++)
    {
      char now = l.base[j];

      if (now == '/' && last == '/')
      {
        // comment stuff to end of line....
        size_t pos_ = j - 1;
        memset(l.style + pos_, COLOR_COMMENT, l.len - pos_);
        break;
      }

      last = now;
    }

    // multiline comment handling
    last = ' ';
    for (size_t j = 0; j < l.len; j++)
    {
      char now = l.base[j];

      if (last == '/' && now == '*')
      {
        in_multiline_comment = true;
        l.style[j - 1] = COLOR_COMMENT;
      }

      if (last == '*' && now == '/')
      {
        in_multiline_comment = false;
        l.style[j] = COLOR_COMMENT;
      }

      if (in_multiline_comment)
      {
        l.style[j] = COLOR_COMMENT;
      }

      last = now;
    }

    // highlighting within the comment handling
    pos = wstart = wend = 0;

    while (CReadWordFromLine(c_comment_word_sep, l.base, l.len, &wstart, &wend, &pos))
    {
      size_t wordlen = wend - wstart;
      const char *kw = NULL;
      int kw_idx = 0;
      for (;;)
      {
        kw = c_comment_keywords[kw_idx];
        kw_idx += 1;

        if (!kw)
          break;

        size_t kwlen = strlen(kw);

        if (kwlen != wordlen)
          continue;

        if (memcmp(kw, l.base + wstart, kwlen) == 0)
        {
          memset(l.style + wstart, COLOR_COMMENT_NOTICE, kwlen);
        }
      }
    }
  }
}
