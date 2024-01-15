#pragma once

namespace utils {

class ListHook {
public:
    bool IsLinked() const {
        return linked;
    }
    void Unlink() {
        if (!IsLinked()) {
            return;
            // ???("Trying to unlink unlinked node");
        }

        ListHook* prev = prev_;
        ListHook* next = next_;
        prev->next_ = next;
        next->prev_ = prev;
        prev_ = this;
        next_ = this;
        linked = false;
    }

    ListHook(const ListHook&) = delete;
    ListHook& operator=(const ListHook&) = delete;

protected:
    ListHook() : next_(this), prev_(this) {
    }

    // Must unlink element from list
    virtual ~ListHook() {
        Unlink();
    }

    // that helper function might be useful
    void LinkBefore(ListHook* other) {
        ListHook* prev = other->prev_;
        next_ = other;
        prev_ = prev;
        prev->next_ = this;
        other->prev_ = this;
        linked = true;
    }

    ListHook* next_;
    ListHook* prev_;
    bool linked = false;

    template <class T>
    friend class List;
};

template <class T>
class List {
public:
    class Iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        Iterator() {
        }
        Iterator(const Iterator& it) = default;
        Iterator(Iterator&& it) = default;
        Iterator& operator=(const Iterator& it) = default;
        Iterator& operator=(Iterator&& it) = default;
        ~Iterator() {
        }

        explicit Iterator(ListHook* val) : current_(val) {
        }

        Iterator& operator++() {
            current_ = current_->prev_;
            return *this;
        }
        Iterator operator++(int) {
            auto temp = *this;
            current_ = current_->prev_;
            return temp;
        }

        Iterator& operator--() {
            current_ = current_->next_;
            return *this;
        }
        Iterator operator--(int) {
            auto temp = *this;
            current_ = current_->next_;
            return temp;
        }

        [[nodiscard]] T& operator*() const {
            return *static_cast<pointer>(current_);
        }
        [[nodiscard]] T* operator->() const {
            return static_cast<pointer>(current_);
        }

        bool operator==(const Iterator& other) const {
            return current_ == other.current_;
        }
        bool operator!=(const Iterator& other) const {
            return current_ != other.current_;
        }

    private:
        ListHook* current_;
    };

    List() : dummy_(new ListHook()) {
    }
    List(const List&) = delete;
    List(List&& other) : dummy_(new ListHook()) {
        std::swap(dummy_, other.dummy_);
    }

    // must unlink all elements from list
    ~List() {
        while (!IsEmpty()) {
            PopFront();
        }
        delete dummy_;
    }

    List& operator=(const List&) = delete;
    List& operator=(List&& other) {
        while (!IsEmpty()) {
            PopFront();
        }
        std::swap(dummy_, other.dummy_);
        return *this;
    }

    [[nodiscard]] bool IsEmpty() const {
        return Size() == 0;
    }
    // this method is allowed to be O(n)
    [[nodiscard]] size_t Size() const {
        size_t size = 0;
        for (ListHook* it = dummy_->prev_; it != dummy_; it = it->prev_) {
            if (it->linked) {
                ++size;
            } else {
                break;
            }
        }

        return size;
    }

    // note that IntrusiveList doesn't own elements,
    // and never copies or moves T
    void PushBack(T* elem) {
        dummy_->linked = true;
        elem->LinkBefore(dummy_->next_);
    }
    void PushFront(T* elem) {
        dummy_->linked = true;
        elem->LinkBefore(dummy_);
    }

    [[nodiscard]] T& Front() {
        return *dynamic_cast<T*>(dummy_->prev_);
    }
    [[nodiscard]] const T& Front() const {
        return *dynamic_cast<T*>(dummy_->prev_);
    }

    [[nodiscard]] T& Back() {
        return *dynamic_cast<T*>(dummy_->next_);
    }
    [[nodiscard]] const T& Back() const {
        return *dynamic_cast<T*>(dummy_->next_);
    }

    void PopBack() {
        ListHook* old = dummy_->next_;
        old->Unlink();
    }
    void PopFront() {
        ListHook* old = dummy_->prev_;
        old->Unlink();
    }

    [[nodiscard]] Iterator Begin() {
        return Iterator(dummy_->prev_);
    }
    [[nodiscard]] Iterator End() {
        return Iterator(dummy_);
    }

    // complexity of this function must be O(1)
    [[nodiscard]] Iterator IteratorTo(T* element) {
        return Iterator(element);
    }

private:
    ListHook* dummy_;
};

template <class T>
[[nodiscard]] List<T>::Iterator begin(List<T>& list) {
    return list.Begin();
}

template <class T>
[[nodiscard]] List<T>::Iterator end(List<T>& list) {
    return list.End();
}
}  // namespace utils