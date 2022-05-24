#pragma once

#include <iterator>
#include <vector>
#include "document.h"

using namespace std::string_literals;

template <typename Iterator>
class Paginator {
    // тело класса
public:
    Paginator(Iterator It_begin, Iterator It_end, int PageSize) {
        _it_begin = It_begin;
        _it_end = It_end;
        _size = PageSize;

        //devide contaner into pages
        int pages_count = std::distance(It_begin, It_end) / PageSize;
        //add full pages except last page
        for (int page = 0; page != pages_count; ++page) {
            auto const _it = _it_begin;
            advance(_it_begin, _size);
            _pages.push_back(std::pair{_it, _it_begin});
        }
        //add last page
        if (_it_begin != _it_end) {
            _pages.push_back(std::pair{_it_begin, _it_end});
        }
    }

    auto begin() const {
        return _pages.begin();
    }

    auto end() const {
        return _pages.end();
    }

    [[nodiscard]] int size() const {
        return _pages.size();
    }
private:
    Iterator _it_begin;
    Iterator _it_end;
    int _size = 0;
    std::vector<std::pair<Iterator, Iterator>> _pages;
};

template <typename Container>
[[maybe_unused]] auto Paginate(const Container& c, std::size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const std::pair<Iterator, Iterator>& container) {
    for (auto document = container.first; document != container.second; ++document) {
        out << *document;
    }
    return out;
}