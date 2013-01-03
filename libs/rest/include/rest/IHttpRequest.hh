// Copyright (C) 2010 - 2012 by Rob Caelers <robc@krandor.nl>
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

#ifndef WORKRAVE_REST_IHTTPREQUEST_HH
#define WORKRAVE_REST_IHTTPREQUEST_HH

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace workrave
{
  namespace rest
  {
    class IHttpRequest : public boost::enable_shared_from_this<IHttpRequest>
    {
    public:
      typedef boost::shared_ptr<IHttpRequest> Ptr;
      typedef std::map<std::string, std::string> Headers;

      static Ptr create();

      virtual ~IHttpRequest() {}
  
    public:
      // TODO: hide publics
      std::string method;
      std::string uri;
      Headers headers;
      std::string body;
      std::string content_type;
    };
  }
}

#endif