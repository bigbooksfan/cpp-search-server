#pragma once

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
                curr.push_back(tmp);
                break;
            }
            curr.push_back(tmp);
        }
    }

public:
    auto begin() const {
        return curr.begin();
    }

    auto end() const {
        return curr.end();
    }

    int size() const {
        return curr.size();
    }

    bool empty() const {
        return curr.size() == 0 ? true : false;
    }

private:

    std::vector<IteratorRange<Iterator>> curr;

};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}