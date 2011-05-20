//
// Urho3D Engine
// Copyright (c) 2008-2011 Lasse ��rni
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "StringBase.h"
#include "Swap.h"

#include <cstdio>

char String::endZero = 0;

String::String(int value) :
    length_(0),
    capacity_(0),
    buffer_(&endZero)
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%d", value);
    *this = tempBuffer;
}

String::String(short value) :
    length_(0),
    capacity_(0),
    buffer_(&endZero)
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%d", value);
    *this = tempBuffer;
}

String::String(unsigned value) :
    length_(0),
    capacity_(0),
    buffer_(&endZero)
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%u", value);
    *this = tempBuffer;
}

String::String(unsigned short value) :
    length_(0),
    capacity_(0),
    buffer_(&endZero)
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%u", value);
    *this = tempBuffer;
}

String::String(float value) :
    length_(0),
    capacity_(0),
    buffer_(&endZero)
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%g", value);
    *this = tempBuffer;
}

String::String(bool value) :
    length_(0),
    capacity_(0),
    buffer_(&endZero)
{
    if (value)
        *this = "true";
    else
        *this = "false";
}

String::String(char value) :
    length_(0),
    capacity_(0),
    buffer_(&endZero)
{
    Resize(1);
    buffer_[0] = value;
}

String::String(char value, unsigned length) :
    length_(0),
    capacity_(0),
    buffer_(&endZero)
{
    Resize(length);
    for (unsigned i = 0; i < length; ++i)
        buffer_[i] = value;
}

String& String::operator += (int rhs)
{
    return *this += String(rhs);
}

String& String::operator += (short rhs)
{
    return *this += String(rhs);
}

String& String::operator += (unsigned rhs)
{
    return *this += String(rhs);
}

String& String::operator += (unsigned short rhs)
{
    return *this += String(rhs);
}

String& String::operator += (float rhs)
{
    return *this += String(rhs);
}

String& String::operator += (bool rhs)
{
    return *this += String(rhs);
}

String String::operator + (int rhs) const
{
    return *this + String(rhs);
}

String String::operator + (short rhs) const
{
    return *this + String(rhs);
}

String String::operator + (unsigned rhs) const
{
    return *this + String(rhs);
}

String String::operator + (unsigned short rhs) const
{
    return *this + String(rhs);
}

String String::operator + (float rhs) const
{
    return *this + String(rhs);
}

String String::operator + (bool rhs) const
{
    return *this + String(rhs);
}

void String::ReplaceInPlace(char replaceThis, char replaceWith)
{
    for (unsigned i = 0; i < length_; ++i)
    {
        if (buffer_[i] == replaceThis)
            buffer_[i] = replaceWith;
    }
}

void String::ReplaceInPlace(const String& replaceThis, const String& replaceWith)
{
    unsigned nextPos = 0;
    
    while (nextPos < length_)
    {
        unsigned pos = Find(replaceThis, nextPos);
        if (pos == NPOS)
            break;
        ReplaceInPlace(pos, replaceThis.length_, replaceWith);
        nextPos = pos + replaceWith.length_;
    }
}

void String::ReplaceInPlace(unsigned pos, unsigned length, const String& str)
{
    // If substring is illegal, do nothing
    if (pos + length > length_)
        return;
    
    ReplaceInPlace(pos, length, str.buffer_, str.length_);
}

String::Iterator String::ReplaceInPlace(const String::Iterator& start, const String::Iterator& end, const String& replaceWith)
{
    unsigned pos = start - Begin();
    if (pos >= length_)
        return End();
    unsigned length = end - start;
    ReplaceInPlace(pos, length, replaceWith);
    
    return Begin() + pos;
}

void String::Insert(unsigned pos, const String& str)
{
    if (pos > length_)
        pos = length_;
    
    if (pos == length_)
        (*this) += str;
    else
        ReplaceInPlace(pos, 0, str);
}

void String::Insert(unsigned pos, char c)
{
    if (pos > length_)
        pos = length_;
    
    if (pos == length_)
        (*this) += c;
    else
    {
        unsigned oldLength = length_;
        Resize(length_ + 1);
        MoveRange(pos + 1, pos, oldLength - pos);
        buffer_[pos] = c;
    }
}

String::Iterator String::Insert(const String::Iterator& dest, const String& str)
{
    unsigned pos = dest - Begin();
    if (pos > length_)
        pos = length_;
    Insert(pos, str);
    
    return Begin() + pos;
}

String::Iterator String::Insert(const String::Iterator& dest, const String::Iterator& start, const String::Iterator& end)
{
    unsigned pos = dest - Begin();
    if (pos > length_)
        pos = length_;
    unsigned length = end - start;
    ReplaceInPlace(pos, 0, &(*start), length);
    
    return Begin() + pos;
}

String::Iterator String::Insert(const String::Iterator& dest, char c)
{
    unsigned pos = dest - Begin();
    if (pos > length_)
        pos = length_;
    Insert(pos, c);
    
    return Begin() + pos;
}

void String::Erase(unsigned pos, unsigned length)
{
    ReplaceInPlace(pos, length, String());
}

String::Iterator String::Erase(const String::Iterator& it)
{
    unsigned pos = it - Begin();
    if (pos >= length_)
        return End();
    Erase(pos);
    
    return Begin() + pos;
}

String::Iterator String::Erase(const String::Iterator& start, const String::Iterator& end)
{
    unsigned pos = start - Begin();
    if (pos >= length_)
        return End();
    unsigned length = end - start;
    Erase(pos, length);
    
    return Begin() + pos;
}

void String::Resize(unsigned newLength)
{
    if (newLength == length_)
        return;
    
    if (!capacity_)
    {
        // Calculate initial capacity
        capacity_ = newLength + 1;
        if (capacity_ < MIN_CAPACITY)
            capacity_ = MIN_CAPACITY;
        
        buffer_ = new char[capacity_];
    }
    else
    {
        if (capacity_ < newLength + 1)
        {
            // Increase the capacity with half each time it is exceeded
            while (capacity_ < newLength + 1)
                capacity_ += (capacity_ + 1) >> 1;
            
            char* newBuffer = new char[capacity_];
            // Move the existing data to the new buffer, then delete the old buffer
            if (length_)
                CopyChars(newBuffer, buffer_, length_);
            delete[] buffer_;
            
            buffer_ = newBuffer;
        }
    }
    
    buffer_[newLength] = 0;
    length_ = newLength;
}

void String::Reserve(unsigned newCapacity)
{
    if (newCapacity < length_ + 1)
        newCapacity = length_ + 1;
    if (newCapacity == capacity_)
        return;
    
    char* newBuffer = new char[newCapacity];
    // Move the existing data to the new buffer, then delete the old buffer
    CopyChars(newBuffer, buffer_, length_ + 1);
    if (capacity_)
        delete[] buffer_;
    
    capacity_ = newCapacity;
    buffer_ = newBuffer;
}

void String::Compact()
{
    if (!capacity_)
        return;
    
    Reserve(length_ + 1);
}

void String::Clear()
{
    Resize(0);
}

void String::Swap(String& str)
{
    ::Swap(length_, str.length_);
    ::Swap(capacity_, str.capacity_);
    ::Swap(buffer_, str.buffer_);
}

String String::Replace(char replaceThis, char replaceWith) const
{
    String ret(*this);
    ret.ReplaceInPlace(replaceThis, replaceWith);
    
    return ret;
}

String String::Replace(const String& replaceThis, const String& replaceWith) const
{
    String ret(*this);
    ret.ReplaceInPlace(replaceThis, replaceWith);
    
    return ret;
}

String String::Replace(unsigned pos, unsigned length, const String& str) const
{
    String ret(*this);
    ret.ReplaceInPlace(pos, length, str);
    
    return ret;
}

String String::Substring(unsigned pos) const
{
    if (pos >= length_)
        return String();
    else
    {
        String ret;
        ret.Resize(length_ - pos);
        CopyChars(ret.buffer_, buffer_ + pos, ret.length_);
        
        return ret;
    }
}

String String::Substring(unsigned pos, unsigned length) const
{
    if (pos >= length_)
        return String();
    else
    {
        String ret;
        if (pos + length > length_)
            length = length_ - pos;
        ret.Resize(length);
        CopyChars(ret.buffer_, buffer_ + pos, ret.length_);
        
        return ret;
    }
}

String String::Trim() const
{
    unsigned trimStart = 0;
    unsigned trimEnd = length_;
    
    while (trimStart < trimEnd)
    {
        char c = buffer_[trimStart];
        if ((c != ' ') && (c != 9))
            break;
        ++trimStart;
    }
    while (trimEnd > trimStart)
    {
        char c = buffer_[trimEnd - 1];
        if ((c != ' ') && (c != 9))
            break;
        --trimEnd;
    }
    
    return Substring(trimStart, trimEnd - trimStart);
}

String String::ToLower() const
{
    String ret(*this);
    for (unsigned i = 0; i < ret.length_; ++i)
        ret[i] = tolower(buffer_[i]);
    
    return ret;
}

String String::ToUpper() const
{
    String ret(*this);
    for (unsigned i = 0; i < ret.length_; ++i)
        ret[i] = toupper(buffer_[i]);
    
    return ret;
}

Vector<String> String::Split(char separator) const
{
    Vector<String> ret;
    unsigned pos = 0;
    
    while (pos < length_)
    {
        unsigned start = pos;
        
        while (start < length_)
        {
            if (buffer_[start] == separator)
                break;
            
            start++;
        }
        
        if (start == length_)
        {
            ret.Push(Substring(pos));
            break;
        }
        
        unsigned end = start;
        
        while (end < length_)
        {
            if (buffer_[end] != separator)
                break;
            
            end++;
        }
        
        ret.Push(Substring(pos, start - pos));
        pos = end;
    }
    
    return ret;
}

unsigned String::Find(char c, unsigned startPos) const
{
    for (unsigned i = startPos; i < length_; ++i)
    {
        if (buffer_[i] == c)
            return i;
    }
    
    return NPOS;
}

unsigned String::Find(const String& str, unsigned startPos) const
{
    if ((!str.length_) || (str.length_ > length_))
        return NPOS;
    
    char first = str.buffer_[0];
    
    for (unsigned i = startPos; i <= length_ - str.length_; ++i)
    {
        if (buffer_[i] == first)
        {
            unsigned skip = NPOS;
            bool found = true;
            for (unsigned j = 1; j < str.length_; ++j)
            {
                char c = buffer_[i + j];
                if ((skip == NPOS) && (c == first))
                    skip = i + j - 1;
                if (c != str.buffer_[j])
                {
                    found = false;
                    if (skip != NPOS)
                        i = skip;
                    break;
                }
            }
            if (found)
                return i;
        }
    }
    
    return NPOS;
}

unsigned String::FindLast(char c) const
{
    for (unsigned i = length_ - 1; i < length_; --i)
    {
        if (buffer_[i] == c)
            return i;
    }
    
    return NPOS;
}

unsigned String::FindLast(const String& str) const
{
    if ((!str.length_) || (str.length_ > length_))
        return NPOS;
    
    char first = str.buffer_[0];
    
    for (unsigned i = length_ - str.length_; i < length_; --i)
    {
        if (buffer_[i] == first)
        {
            bool found = true;
            for (unsigned j = 1; j < str.length_; ++j)
            {
                char c = buffer_[i + j];
                if (c != str.buffer_[j])
                {
                    found = false;
                    break;
                }
            }
            if (found)
                return i;
        }
    }
    
    return NPOS;
}

void String::ReplaceInPlace(unsigned pos, unsigned length, const char* srcStart, unsigned srcLength)
{
    int delta = (int)srcLength - (int)length;
    
    if (pos + length < length_)
    {
        if (delta < 0)
        {
            MoveRange(pos + srcLength, pos + length, length_ - pos - length);
            Resize(length_ + delta);
        }
        if (delta > 0)
        {
            Resize(length_ + delta);
            MoveRange(pos + srcLength, pos + length, length_ - pos - length);
        }
    }
    else
        Resize(length_ + delta);
    
    CopyChars(buffer_ + pos, srcStart, srcLength);
}