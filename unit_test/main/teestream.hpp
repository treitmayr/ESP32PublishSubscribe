// Implementation from https://wordaligned.org/articles/cpp-streambufs

#include <teebuf.hpp>

class teestream : public std::ostream
{
public:
    // Construct an ostream which tees output to the supplied
    // ostreams.
    teestream(std::ostream& o1, std::ostream& o2) :
        std::ostream(&tbuf), tbuf(o1.rdbuf(), o2.rdbuf())
    {}

private:
    teebuf tbuf;
};
