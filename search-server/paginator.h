#pragma once

#include <vector>

template<typename Iterator>
class IteratorRange {                   // kinda wrapper
public:
    IteratorRange(Iterator begin, Iterator end) : begin_{ begin }, end_{ end } {};
    IteratorRange() {

    }

public:
    Iterator begin() const {
        return begin_;
    }

    Iterator end() const {
        return end_;
    }

    Iterator size() const {
        return end_ - begin_;
    }

private:
    Iterator begin_;
    Iterator end_;

};

template<typename Iterator>
std::ostream& operator<< (std::ostream& os, const IteratorRange<Iterator>& out) {
    for (Iterator It = out.begin(); It < out.end(); ++It) os << *It;
    return os;
}

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        IteratorRange<Iterator> tmp;
        for (Iterator It = begin; It < end;) {
            if (It < end - page_size) {
                tmp = IteratorRange<Iterator>(It, It + page_size);
                It += page_size;
            }
            else {
                tmp = IteratorRange<Iterator>(It, end);
                curr_.push_back(tmp);
                break;
            }
            curr_.push_back(tmp);
        }
    }

public:
    auto begin() const {
        return curr_.begin();
    }

    auto end() const {
        return curr_.end();
    }

    int size() const {
        return curr_.size();
    }

    bool empty() const {
        return curr_.size() == 0 ? true : false;
    }

private:

    std::vector<IteratorRange<Iterator>> curr_;

};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}