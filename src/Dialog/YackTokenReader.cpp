#include <algorithm>
#include "Dialog/YackTokenReader.h"

namespace ng
{
std::string Token::readToken() const
{
    switch (id)
    {
    case TokenId::Assign:
        return "Assign";
    case TokenId::NewLine:
        return "NewLine";
    case TokenId::Colon:
        return "Colon";
    case TokenId::Code:
        return "Code";
    case TokenId::Comment:
        return "Comment";
    case TokenId::End:
        return "End";
    case TokenId::Goto:
        return "Goto";
    case TokenId::Identifier:
        return "Identifier";
    case TokenId::None:
        return "None";
    case TokenId::Number:
        return "Number";
    case TokenId::Condition:
        return "Condition";
    case TokenId::String:
        return "String";
    case TokenId::Whitespace:
        return "Whitespace";
    default:
        return "?";
    }
}

YackTokenReader::Iterator::Iterator(YackTokenReader &reader, std::streampos pos)
    : _reader(reader),
      _pos(pos)
{
}

YackTokenReader::Iterator::Iterator(const Iterator &it)
    : _reader(it._reader), _pos(it._pos), _token(it._token)
{
}

YackTokenReader::Iterator &YackTokenReader::Iterator::operator++()
{
    _reader._stream.seek(_pos);
    _reader.readToken(_token);
    _pos = _reader._stream.tell();
    return *this;
}

YackTokenReader::Iterator YackTokenReader::Iterator::operator++(int)
{
    Iterator tmp(*this);
    operator++();
    return tmp;
}

Token &YackTokenReader::Iterator::operator*()
{
    return _token;
}

Token *YackTokenReader::Iterator::operator->()
{
    return &_token;
}

void YackTokenReader::load(const std::string &path)
{
    std::vector<char> buffer;
    _pSettings->readEntry(path, buffer);
    _stream.setBuffer(buffer);
}

YackTokenReader::iterator YackTokenReader::begin()
{
    return Iterator(*this, 0);
}
YackTokenReader::iterator YackTokenReader::end()
{
    auto start = _stream.tell();
    _stream.seek(_stream.getLength());
    auto pos = _stream.tell();
    _stream.seek(start);
    return Iterator(*this, pos);
}

YackTokenReader::YackTokenReader()
    : _pSettings(nullptr)
{
}

void YackTokenReader::setSettings(EngineSettings &settings)
{
    _pSettings = &settings;
}

bool YackTokenReader::readToken(Token &token)
{
    std::streampos start = _stream.tell();
    auto id = readTokenId();
    while (id == TokenId::Whitespace || id == TokenId::Comment)
    {
        start = _stream.tell();
        id = readTokenId();
    }
    std::streampos end = _stream.tell();
    token.id = id;
    token.start = start;
    token.end = end;
    return true;
}

std::string YackTokenReader::readText(std::streampos pos, std::streamsize size)
{
    std::string out;
    out.reserve(size);
    _stream.seek(pos);
    char c;
    for (int i = 0; i < size; i++)
    {
        _stream.read(&c, 1);
        out.append(&c, 1);
    }
    return out;
}

std::string YackTokenReader::readText(const Token &token)
{
    return readText(token.start, token.end - token.start);
}

TokenId YackTokenReader::readTokenId()
{
    char c;
    _stream.read(&c, 1);
    if (_stream.eof())
    {
        return TokenId::End;
    }

    switch (c)
    {
    case '\0':
        return TokenId::End;
    case '\n':
        return TokenId::NewLine;
    case '\t':
    case ' ':
        while (isspace(_stream.peek()) && _stream.peek() != '\n')
            _stream.ignore();
        return TokenId::Whitespace;
    case '!':
        return readCode();
    case ':':
        return TokenId::Colon;
    case '[':
        return readCondition();
    case '=':
        return TokenId::Assign;
    case '\"':
        return readString();
    case '#':
    case ';':
        return readComment();
    default:
        if (c == '-' && _stream.peek() == '>')
        {
            _stream.ignore();
            return TokenId::Goto;
        }
        if (c == '-' || isdigit(c))
        {
            return readNumber();
        }
        else if (isalpha(c))
        {
            return readIdentifier();
        }
        std::cerr << "unknown character: " << c << std::endl;
        return TokenId::None;
    }
}

TokenId YackTokenReader::readCode()
{
    char c;
    while ((c = _stream.peek()) != '[' && c != '\n' && c != '\0')
    {
        _stream.ignore();
    }
    return TokenId::Code;
}

TokenId YackTokenReader::readCondition()
{
    char c;
    while (_stream.peek() != ']')
    {
        _stream.ignore();
    }
    _stream.ignore();
    return TokenId::Condition;
}

TokenId YackTokenReader::readNumber()
{
    char c;
    while (isdigit(_stream.peek()))
    {
        _stream.ignore();
    }
    if (_stream.peek() == '.')
    {
        _stream.ignore();
    }
    while (isdigit(_stream.peek()))
    {
        _stream.ignore();
    }
    return TokenId::Number;
}

TokenId YackTokenReader::readComment()
{
    _stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return TokenId::Comment;
}

TokenId YackTokenReader::readString()
{
    _stream.ignore(std::numeric_limits<std::streamsize>::max(), '\"');
    return TokenId::String;
}

TokenId YackTokenReader::readIdentifier()
{
    char c;
    while (isalnum(_stream.peek()) || _stream.peek() == '_')
    {
        _stream.ignore();
    }
    return TokenId::Identifier;
}
} // namespace ng
