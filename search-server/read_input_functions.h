#pragma once

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