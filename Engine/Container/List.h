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

#pragma once

#include "ListBase.h"

/// Linked list template class
template <class T> class List : public ListBase
{
public:
    /// List node
    struct Node : public ListNodeBase
    {
        /// Construct undefined
        Node()
        {
        }
        
        /// Construct with value
        Node(const T& value) :
            value_(value)
        {
        }
        
        /// Node value
        T value_;
        
        /// Return next node
        Node* GetNext() const { return static_cast<Node*>(next_); }
        
        /// Return previous node
        Node* GetPrev() { return static_cast<Node*>(prev_); }
    };
    
    /// List node iterator
    class Iterator : public ListIteratorBase
    {
    public:
        /// Construct
        explicit Iterator(Node* ptr) :
            ListIteratorBase(ptr)
        {
        }
        
        /// Preincrement the pointer
        Iterator& operator ++ () { GotoNext(); return *this; }
        /// Postincrement the pointer
        Iterator operator ++ (int) { Iterator it = *this; GotoNext(); return it; }
        /// Predecrement the pointer
        Iterator& operator -- () { GotoPrev(); return *this; }
        /// Postdecrement the pointer
        Iterator operator -- (int) { Iterator it = *this; GotoPrev(); return it; }
        
        /// Point to the node value
        T* operator -> () const { return &(static_cast<Node*>(ptr_))->value_; }
        /// Dereference the node value
        T& operator * () const { return (static_cast<Node*>(ptr_))->value_; }
    };
    
    /// List node const iterator
    class ConstIterator : public ListIteratorBase
    {
    public:
        /// Construct
        explicit ConstIterator(Node* ptr) :
            ListIteratorBase(ptr)
        {
        }
        
        /// Construct from a non-const iterator
        ConstIterator(const Iterator& rhs) :
            ListIteratorBase(rhs.ptr_)
        {
        }
        
        /// Assign from a non-const iterator
        ConstIterator& operator = (const Iterator& rhs) { ptr_ = rhs.ptr_; return *this; }
        /// Preincrement the pointer
        ConstIterator& operator ++ () { GotoNext(); return *this; }
        /// Postincrement the pointer
        ConstIterator operator ++ (int) { Iterator it = *this; GotoNext(); return it; }
        /// Predecrement the pointer
        ConstIterator& operator -- () { GotoPrev(); return *this; }
        /// Postdecrement the pointer
        ConstIterator operator -- (int) { Iterator it = *this; GotoPrev(); return it; }
        
        /// Point to the node value
        const T* operator -> () const { return &(static_cast<Node*>(ptr_))->value_; }
        /// Dereference the node value
        const T& operator * () const { return (static_cast<Node*>(ptr_))->value_; }
    };

    /// Construct empty
    List() :
        ListBase()
    {
        // Allocate the tail node
        allocator_ = AllocatorInitialize(sizeof(Node));
        head_ = tail_ = AllocateNode();
    }
    
    /// Construct from another list
    List(const List<T>& list) :
        ListBase()
    {
        // Allocate the tail node
        allocator_ = AllocatorInitialize(sizeof(Node));
        head_ = tail_ = AllocateNode();
        
        // Then assign the other list
        *this = list;
    }
    
    /// Destruct
    ~List()
    {
        Clear();
        FreeNode(GetTail());
        AllocatorUninitialize(allocator_);
    }
    
    /// Assign from another list
    List& operator = (const List<T>& rhs)
    {
        // Clear, then insert the nodes of the other list
        Clear();
        Insert(End(), rhs.Begin(), rhs.End());
        
        return *this;
    }
    
    /// Add-assign a value
    List& operator += (const T& rhs)
    {
        Push(rhs);
        return *this;
    }
    
    /// Add-assign a list
    List& operator += (const List<T>& rhs)
    {
        Insert(End(), rhs.Begin(), rhs.End());
        return *this;
    }
    
    /// Test for equality with another list
    bool operator == (const List<T>& rhs) const
    {
        if (rhs.size_ != size_)
            return false;
        
        ConstIterator i = Begin();
        ConstIterator j = rhs.Begin();
        while (i != End())
        {
            if (*i != *j)
                return false;
            ++i;
            ++j;
        }
        
        return true;
    }
    
    /// Test for inequality with another list
    bool operator != (const List<T>& rhs) const
    {
        if (rhs.size_ != size_)
            return true;
        
        ConstIterator i = Begin();
        ConstIterator j = rhs.Begin();
        while (i != End())
        {
            if (*i != *j)
                return true;
            ++i;
            ++j;
        }
        
        return false;
    }
    
    /// Insert a value at the end
    void Push(const T& value)
    {
        InsertNode(GetTail(), value);
    }
    
    /// Insert a value into the list
    void Insert(const Iterator& dest, const T& value)
    {
        InsertNode(static_cast<Node*>(dest.ptr_), value);
    }
    
    /// Insert a range by iterators
    void Insert(const Iterator& dest, const Iterator& start, const Iterator& end)
    {
        Node* destNode = static_cast<Node*>(dest.ptr_);
        Iterator it = start;
        while (it != end)
            InsertNode(destNode, *it++);
    }
    
    /// Erase the last node
    void Pop()
    {
        if (size_)
            Erase(End() - 1);
    }
    
    /// Erase a node from the list. Return an iterator to the next element
    Iterator Erase(Iterator it)
    {
        return Iterator(EraseNode(static_cast<Node*>(it.ptr_)));
    }
    
    /// Erase a range of nodes from the list. Return an iterator to the next element
    Iterator Erase(const Iterator& start, const Iterator& end)
    {
        Iterator it = start;
        while (it != end)
            it = EraseNode(static_cast<Node*>(it.ptr_));
        
        return it;
    }
    
    /// Clear the list
    void Clear()
    {
        while (size_)
            EraseNode(GetHead());
    }
    
    /// Return iterator to the first node
    Iterator Begin() { return Iterator(GetHead()); }
    /// Return iterator to the first node
    ConstIterator Begin() const { return ConstIterator(GetHead()); }
    /// Return iterator to the end
    Iterator End() { return Iterator(GetTail()); }
    /// Return iterator to the end
    ConstIterator End() const { return ConstIterator(GetTail()); }
    /// Return first value
    const T& Front() const { return *Begin(); }
    /// Return last value
    const T& Back() const { return *(--End()); }
    /// Return number of value
    unsigned Size() const { return size_; }
    /// Return whether list is empty
    bool Empty() const { return size_ == 0; }
    
private:
    /// Return the head pointer with correct type
    Node* GetHead() const { return reinterpret_cast<Node*>(head_); }
    /// Return the tail pointer with correct type
    Node* GetTail() const { return reinterpret_cast<Node*>(tail_); }
    
    /// Insert a value into the list
    void InsertNode(Node* dest, const T& value)
    {
        if (!dest)
            return;
        
        Node* newNode = AllocateNode(value);
        Node* prev = dest->GetPrev();
        newNode->next_ = dest;
        newNode->prev_ = prev;
        if (prev)
            prev->next_ = newNode;
        dest->prev_ = newNode;
        
        
        // Reassign the head node if necessary
        if (dest == GetHead())
            head_ = newNode;
        
        ++size_;
    }
    
    /// Erase a node from the list. Return pointer to the next element, or to the end if could not erase
    Node* EraseNode(Node* toRemove)
    {
        // The tail node can not be removed
        if ((!toRemove) || (toRemove == tail_))
            return GetTail();
        
        Node* prev = toRemove->GetPrev();
        Node* next = toRemove->GetNext();
        if (prev)
            prev->next_ = next;
        next->prev_ = prev;
        
        // Reassign the head node if necessary
        if (toRemove == GetHead())
            head_ = next;
        
        FreeNode(toRemove);
        --size_;
        
        return next;
    }
    
    /// Allocate a node
    Node* AllocateNode()
    {
        Node* newNode = static_cast<Node*>(AllocatorReserve(allocator_));
        new(newNode) Node();
        return newNode;
    }
    
    /// Allocate a node with initial value
    Node* AllocateNode(const T& value)
    {
        Node* newNode = static_cast<Node*>(AllocatorReserve(allocator_));
        new(newNode) Node(value);
        return newNode;
    }
    
    /// Free a node
    void FreeNode(Node* node)
    {
        (node)->~Node();
        AllocatorFree(allocator_, node);
    }
};