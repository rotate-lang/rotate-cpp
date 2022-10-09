#include "lexer.hpp"

namespace rotate
{

// file must not be null and lexer owns the file ptr
Lexer::Lexer(file_t *file)
    : index(0), len(0), line(1), file(file), file_length(file ? file->length : 0), is_done(false),
      error(error_type::UNKNOWN), tokens(new std::vector<Token>())
{
    ASSERT_NULL(file, "Lexer File passed is a null pointer");
    ASSERT_NULL(tokens, "Lexer vec of tokens passed is a null pointer");
    tokens->reserve(file->length / 2);
}

Lexer::~Lexer() noexcept
{
    delete tokens;
}

void Lexer::save_log(FILE *output)
{
    for (usize i = 0; i < tokens->size(); i++)
    {
        log_token(output, tokens->at(i), file->contents);
    }
}

u8 Lexer::lex()
{
    do
    {
        switch (lex_director())
        {
            case EXIT_SUCCESS:
                break;
            case EXIT_DONE:
                len = 0;
                add_token(token_type::EOT);
                return EXIT_SUCCESS;
            case EXIT_FAILURE:
                return report_error();
        }
    } while (true);
    return EXIT_SUCCESS;
}

inline void Lexer::skip_whitespace() noexcept
{
    while (true)
    {
        const char c = current();
        if (c == '\n')
        {
            index++;
            line++;
        }
        else if (c == ' ')
        {
            index++;
        }
        else
            break;
    }
}

u8 Lexer::lex_director()
{
    skip_whitespace();
    len = 0, begin_tok_line = line;

    const char c = current();

    if (isdigit(c)) return lex_numbers();

    //
    if (c == '\'') return lex_chars();
    if (c == '"') return lex_strings();

    //
    if (c == '_' || isalpha(c)) return lex_identifiers();
    if (c == '@') return lex_builtin_funcs();

    return lex_symbols();
}

std::vector<Token> *Lexer::getTokens()
{
    return tokens;
}

file_t *Lexer::getFile()
{
    return file;
}

u8 Lexer::lex_identifiers()
{
    advance_len_inc();
    while (isalnum(current()) || current() == '_')
    {
        advance_len_inc();
    }
    index -= len;
    token_type _type = token_type::Identifier;

    // TODO: optimize matching keywords
    switch (len)
    {
        case 2: {
            switch (current())
            {
                case 'f':
                    if (keyword_match("fn", 2)) _type = token_type::Function;
                    break;
                case 'i':
                    if (keyword_match("if", 2))
                        _type = token_type::If;
                    else if (keyword_match("in", 2))
                        _type = token_type::In;
                    break;
                case 'o':
                    if (keyword_match("or", 2)) _type = token_type::Or;
                    break;
                default:
                    break;
            }

            break;
        }
        case 3: {
            switch (current())
            {
                case 'f':
                    if (keyword_match("for", 3)) _type = token_type::For;
                    break;
                case 'p':
                    if (keyword_match("pub", 3)) _type = token_type::Public;
                    break;
                case 'i':
                    if (keyword_match("int", 3)) _type = token_type::IntKeyword;
                    break;
                case 'r':
                    if (keyword_match("ref", 3)) _type = token_type::Ref;
                    break;
                case 'a':
                    if (keyword_match("and", 3)) _type = token_type::And;
                    break;
                case 'n':
                    if (keyword_match("nil", 3)) _type = token_type::Nil;
                    break;
            }
            break;
        }
        case 4: {
            switch (current())
            {
                case 'e': {
                    if (keyword_match("else", 4))
                        _type = token_type::Else;
                    else if (keyword_match("enum", 4))
                        _type = token_type::Enum;
                    break;
                }
                case 't':
                    if (keyword_match("true", 4)) _type = token_type::True;
                    break;
                case 'c':
                    if (keyword_match("char", 4)) _type = token_type::CharKeyword;
                    break;
                case 'b':
                    if (keyword_match("bool", 4)) _type = token_type::BoolKeyword;
                    break;
                case 'u':
                    if (keyword_match("uint", 4)) _type = token_type::UintKeyword;
                    break;
            }
            break;
        }
        case 5: {
            switch (current())
            {
                case 'w':
                    if (keyword_match("while", 5)) _type = token_type::While;
                    break;
                case 'f': {
                    if (keyword_match("false", 5))
                        _type = token_type::False;
                    else if (keyword_match("float", 5))
                        _type = token_type::FloatKeyword;
                    break;
                }
                case 'b':
                    if (keyword_match("break", 5)) _type = token_type::Break;
                    break;
            }
            break;
        }
        case 6: {
            switch (current())
            {
                case 'r':
                    if (keyword_match("return", 6)) _type = token_type::Return;
                    break;
                case 'i':
                    if (keyword_match("import", 6)) _type = token_type::Import;
                    break;
                case 'd':
                    if (keyword_match("delete", 6)) _type = token_type::Delete;
                    break;
                case 's': {
                    if (keyword_match("struct", 6))
                        _type = token_type::Struct;
                    else if (keyword_match("switch", 6))
                        _type = token_type::Switch;
                    break;
                }
            }
            break;
        }
        default:
            break;
    }

    if (len > 100)
    {
        // log_error("identifier length is more than 128 chars");
        error = error_type::TOO_LONG_IDENTIFIER;
        return EXIT_FAILURE;
    }

    return add_token(_type);
}

u8 Lexer::lex_numbers()
{
    // const char c = current();
    // const char p = peek();
    // if (c == '0' && p == 'x') return lex_hex_numbers();
    // if (c == '0' && p == 'b') return lex_binary_numbers();

    bool reached_dot = false;
    while (isdigit(current()) || current() == '.')
    {
        advance_len_inc();
        if (current() == '.')
        {
            if (reached_dot) break;
            reached_dot = true;
        }
    }

    if (len > 100)
    {
        log_error("number digits length is above 100");
        return EXIT_FAILURE;
    }
    index -= len;
    return add_token(reached_dot ? token_type::Float : token_type::Integer);
}

/*
u8 Lexer::lex_hex_numbers()
{
    // skip '0x'
    advance();
    advance();
    while (isxdigit(current()))
    {
        advance_len_inc();
    }

    if (len > 32)
    {
        log_error("hex number digits length is above 32");
    }
    index -= len;
    return add_token(token_type::HexInteger);
}

u8 Lexer::lex_binary_numbers()
{
    // skip '0b'
    advance();
    advance();
    while (current() == '0' || current() == '1')
    {
        advance_len_inc();
    }

    if (len > 128)
    {
        log_error("binary number digits length is above 128");
    }
    index -= len;
    return add_token(token_type::BinaryInteger);
} */

u8 Lexer::lex_strings()
{
    advance_len_inc();
    while (current() != '"' && past() != '\\')
    {
        if (current() == '\0')
        {
            reverse_len_for_error();
            error = error_type::NOT_CLOSED_STRING;
            return EXIT_FAILURE;
        }
        advance_len_inc();
    }
    advance_len_inc();

    if (len > UINT16_MAX)
    {
        log_error("Too long string");
    }
    index -= len++;

    return add_token(token_type::String);
}

u8 Lexer::lex_chars()
{
    advance_len_inc();
    if (current() != '\\' && peek() == '\'')
    {
        advance_len_inc();
        advance_len_inc();
        index -= len;
        return add_token(token_type::Char);
    }
    else if (current() == '\\')
    {
        advance_len_inc();
        switch (current())
        {
            case 'n':
            case 't':
            case 'r':
            case 'b':
            case 'f':
            case '\\':
            case '\'':
                advance_len_inc();
                break;
            default:
                error = error_type::NOT_VALID_ESCAPE_CHAR;
                return reverse_len_for_error();
        }
        if (current() == '\'')
        {
            advance_len_inc();
            index -= len;
            return add_token(token_type::Char);
        }
        else
        {
            error = error_type::LEXER_INVALID_CHAR;
            return reverse_len_for_error();
        }
    }

    return EXIT_FAILURE;
}

u8 Lexer::lex_symbols()
{
    const char c = current();
    const char p = peek();
    len          = 1;
    switch (c)
    {
            // clang-format off
        case '{': return add_token(token_type::OpenCurly);
        case '}': return add_token(token_type::CloseCurly);
        case '(': return add_token(token_type::OpenParen);
        case ')': return add_token(token_type::CloseParen);
        case '[': return add_token(token_type::OpenSQRBrackets);
        case ']': return add_token(token_type::CloseSQRBrackets);
        case ';': return add_token(token_type::SemiColon);
        case ',': return add_token(token_type::Comma);
        // TODO(5717) bug below needs to check an eql during peeking
        // clang-format on
        case '.': {
            if (p == '.')
            {
                len++;
                return add_token(token_type::To);
            }
            return add_token(token_type::Dot);
        }
        case ':': {
            if (p == ':')
            {
                len++;
                return add_token(token_type::ColonColon);
            }
            return add_token(token_type::Colon);
        }
        case '>': {
            if (p == '=')
            {
                len++;
                return add_token(token_type::GreaterEql);
            }
            return add_token(token_type::Greater);
        }
        case '<': {
            if (p == '=')
            {
                len++;
                return add_token(token_type::LessEql);
            }
            return add_token(token_type::Less);
        }
        case '=': {
            if (p == '=')
            {
                len++;
                return add_token(token_type::EqualEqual);
            }
            return add_token(token_type::Equal);
        }
        case '+': {
            if (p == '=')
            {
                len++;
                return add_token(token_type::AddEqual);
            }
            return add_token(token_type::PLUS);
        }
        case '-': {
            if (p == '=')
            {
                len++;
                return add_token(token_type::SubEqual);
            }
            return add_token(token_type::MINUS);
        }
        case '*': {
            if (p == '=')
            {
                len++;
                return add_token(token_type::MultEqual);
            }
            return add_token(token_type::Star);
        }
        case '/': {
            if (p == '=')
            {
                len++;
                return add_token(token_type::DivEqual);
            }
            else if (p == '/')
            {
                //
                while (is_not_eof() && current() != '\n')
                    advance();
                return EXIT_SUCCESS;
            }
            else if (p == '*')
            {
                advance();
                // TODO: Allow nested comments
                bool end_comment = false;
                while (is_not_eof() && !end_comment)
                {
                    if ((past() != '/' && current() == '*' && peek() == '/'))
                    {
                        advance();
                        end_comment = true;
                    }
                    advance();
                }
                return EXIT_SUCCESS;
            }
            return add_token(token_type::DIV);
        }
        case '!': {
            if (p == '=')
            {
                len++;
                return add_token(token_type::NotEqual);
            }
            return add_token(token_type::Not);
        }
        default: {
            switch (c)
            {
                case '\0':
                    is_done = true;
                    return EXIT_DONE;
                case '\t':
                    this->error = error_type::TABS;
                    break;
                case '\r':
                    this->error = error_type::WINDOWS_CRAP;
                    break;
                default:
                    this->error = error_type::LEXER_INVALID_CHAR;
            }
        }
    }
    return EXIT_FAILURE;
}

u8 Lexer::lex_builtin_funcs()
{
    advance(); // skip '@'
    while (isalpha(current()))
    {
        advance_len_inc();
    }
    index -= len;

    switch (len)
    {
        case 3: {
            if (keyword_match("col", 3))
            {
                return add_token(token_type::BuiltinFunc);
            }
            break;
        }
        case 4: {
            if (keyword_match("line", 4))
            {
                return add_token(token_type::BuiltinFunc);
            }
            else if (keyword_match("file", 4))
            {
                return add_token(token_type::BuiltinFunc);
            }
            break;
        }
        case 5: {
            if (keyword_match("print", 5))
            {
                return add_token(token_type::BuiltinFunc);
            }
            break;
        }
        case 7: {
            if (keyword_match("println", 7))
            {
                return add_token(token_type::BuiltinFunc);
            }
            break;
        }
        default: {
            break;
        }
    }

    error = error_type::LEXER_INVALID_BUILTN_FN;
    return reverse_len_for_error();
}

inline void Lexer::advance()
{
    const char c = current();
    index++;
    if (c == '\n') line++;
}

inline void Lexer::advance_len_times()
{
    index += len;
}

inline void Lexer::advance_len_inc()
{
    const char c = current();
    index++;
    len++;
    line += c == '\n'; // if (c == '\n') line++;
}

inline char Lexer::peek() const
{
    return file->contents[index + 1];
}

inline char Lexer::current() const
{
    return file->contents[index];
}

inline char Lexer::past() const
{
    return file->contents[index - 1];
}

inline bool Lexer::is_not_eof() const
{
    return index < file_length;
}

inline bool Lexer::keyword_match(const char *string, u32 length)
{
    return strncmp(file->contents + index, string, length) == 0;
}

u8 Lexer::reverse_len_for_error()
{
    index -= len;
    return EXIT_FAILURE;
}

u8 Lexer::report_error()
{
    //
    u32 low = index, col = 0;
    while (file->contents[low] != '\n' && low > 0)
    {
        low--;
        col++;
    }
    low++;

    //
    u32 _length = index;
    while (file->contents[_length] != '\n' && _length + 1 < file->length)
        _length++;

    _length -= low;

    // error msg
    fprintf(stderr, "> %s%s%s:%u:%u: %serror: %s%s%s\n", BOLD, WHITE, file->name, line, col, LRED,
            LBLUE, err_msgsfunc(error), RESET);

    // line from source code
    fprintf(stderr, " %s%u%s | %.*s\n", LYELLOW, line, RESET, _length, (file->contents + low));

    u32 num_line_digits = get_digits_from_number(line);

    // arrows pointing to error location
    u32 spaces = index - low + 1;
    if (len < 101)
    {
        char *arrows = (char *)alloca(len + 1);
        memset(arrows, '^', len);
        arrows[len] = '\0';

        fprintf(stderr, " %*c |%*c%s%s%s\n", num_line_digits, ' ', spaces, ' ', LRED, BOLD, arrows);
    }
    else
    {
        fprintf(stderr, " %*c |%*c%s%s^^^---...\n", num_line_digits, ' ', spaces, ' ', LRED, BOLD);
    }
    // error advice
    fprintf(stderr, "> Advice: %s%s\n", RESET, advice(error));
    return EXIT_FAILURE;
}

u8 Lexer::add_token(const token_type type)
{
    // index at the end of the token
    // NOTE(Airbus5717): emplace_back constructs the token in the vector
    tokens->emplace_back(type, index, len, begin_tok_line);

    for (u32 i = 0; i < len; i++)
        advance();

    return EXIT_SUCCESS;
}

} // namespace rotate
