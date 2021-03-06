#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <iostream>
#include <exception>
#include <sstream>
#include <string>

class CException : public std::exception
{
public:

    explicit
    CException(const std::string && message = "", const std::string && file = "", const int line = 0):
        mmsg(std::move(message)), mfile(std::move(file)), mline(line)
    {}
    CException(const CException & cexcep) :
        mmsg(cexcep.message()), mfile(cexcep.fileName()), mline(cexcep.lineNumber())
    {}
    CException(const std::exception & excep) :
        mmsg(excep.what()), mfile(""), mline(0)
    {}

    virtual ~CException() _NOEXCEPT {}

    virtual const char* what() const
    {
        if (mmsg.empty()) return "";
        std::ostringstream ss;
        if (!mfile.empty()) ss << mfile << ": line " << mline << ": " << mmsg;
        else ss << mmsg;
        return ss.str().c_str();
    }

    const std::string & message() const { return mmsg; }
    const std::string & fileName() const { return mfile; }
    int lineNumber() const { return mline; }

    virtual void raise() const { throw *this; }
    virtual CException *clone() const { return new CException(*this); }

    friend std::ostream & operator << (std::ostream &rOutputStream, const CException &rException)
    {
        rOutputStream << rException.what();
        return rOutputStream;
    }

private:
    std::string mfile;
    int mline;
    std::string mmsg;
};

#define FILE_AND_LINE __FILE__, __LINE__
#define THROW_EXCEP(errString) throw CException(errString, FILE_AND_LINE)
#define ASSERT_MSG(C, message) do {if (!(C)) THROW_EXCEP(message);} while(false)
#define ASSERT(C) ASSERT_MSG(C, "Assertion failure: " #C)

#define ASSERT_AUTO(C) do { \
    if (!C) {   \
    std::cerr << "\nException in line " << __LINE__ << " of \""   \
    << __FILE__ << "\"\nGenerated by \"" #C << "\"\n"; \
    THROW_EXCEP("");}} while (false)

#endif // EXCEPTION_H
