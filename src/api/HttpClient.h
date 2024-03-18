#ifndef Advanced_HttpClient_h
#define Advanced_HttpClient_h

//#ifndef HTTP_METHOD_GET
#define HTTP_METHOD_GET "GET"
#define HTTP_METHOD_POST "POST"
#define HTTP_METHOD_PUT "PUT"
#define HTTP_METHOD_PATCH "PATCH"
#define HTTP_METHOD_DELETE "DELETE"

// static const int HTTP_SUCCESS =0;
// // The end of the headers has been reached.  This consumes the '\n'
// // Could not connect to the server
// static const int HTTP_ERROR_CONNECTION_FAILED =-1;
// // This call was made when the HttpClient class wasn't expecting it
// // to be called.  Usually indicates your code is using the class
// // incorrectly
// static const int HTTP_ERROR_API =-2;
// // Spent too long waiting for a reply
// static const int HTTP_ERROR_TIMED_OUT =-3;
// // The response from the server is invalid, is it definitely an HTTP
// // server?
// static const int HTTP_ERROR_INVALID_RESPONSE =-4;

#define HTTP_SUCCESS 0;
#define HTTP_ERROR_CONNECTION_FAILED -1;
#define HTTP_ERROR_API -2;
#define HTTP_ERROR_TIMED_OUT -3;
#define HTTP_ERROR_INVALID_RESPONSE -4;

#include <Arduino.h>
#include <Client.h>

class HttpClient {
 public:
  static const int HttpPort = 80;
  static const int HttpsPort = 443;

  /** Connect to the server and start to send a GET request.
    @param aURLPath     Url to request
    @return 0 if successful, else error
  */
  virtual int get(const char* aURLPath) = 0;
  virtual int get(const String& aURLPath) = 0;

  /** Connect to the server and start to send a POST request.
    @param aURLPath     Url to request
    @return 0 if successful, else error
  */
  virtual int post(const char* aURLPath) = 0;
  virtual int post(const String& aURLPath) = 0;

  /** Connect to the server and send a POST request
      with body and content type
    @param aURLPath     Url to request
    @param aContentType Content type of request body
    @param aBody        Body of the request
    @return 0 if successful, else error
  */
  virtual int post(const char* aURLPath,
                   const char* aContentType,
                   const char* aBody) = 0;
  virtual int post(const String& aURLPath,
                   const String& aContentType,
                   const String& aBody) = 0;
  virtual int post(const char* aURLPath,
                   const char* aContentType,
                   int aContentLength,
                   const byte aBody[]) = 0;

  /** Connect to the server and start to send a PUT request.
    @param aURLPath     Url to request
    @return 0 if successful, else error
  */
  virtual int put(const char* aURLPath) = 0;
  virtual int put(const String& aURLPath) = 0;

  /** Connect to the server and send a PUT request
      with body and content type
    @param aURLPath     Url to request
    @param aContentType Content type of request body
    @param aBody        Body of the request
    @return 0 if successful, else error
  */
  virtual int put(const char* aURLPath,
                  const char* aContentType,
                  const char* aBody) = 0;
  virtual int put(const String& aURLPath,
                  const String& aContentType,
                  const String& aBody) = 0;
  virtual int put(const char* aURLPath,
                  const char* aContentType,
                  int aContentLength,
                  const byte aBody[]) = 0;

  /** Connect to the server and start to send a PATCH request.
    @param aURLPath     Url to request
    @return 0 if successful, else error
  */
  virtual int patch(const char* aURLPath) = 0;
  virtual int patch(const String& aURLPath) = 0;

  /** Connect to the server and send a PATCH request
      with body and content type
    @param aURLPath     Url to request
    @param aContentType Content type of request body
    @param aBody        Body of the request
    @return 0 if successful, else error
  */
  virtual int patch(const char* aURLPath,
                    const char* aContentType,
                    const char* aBody) = 0;
  virtual int patch(const String& aURLPath,
                    const String& aContentType,
                    const String& aBody) = 0;
  virtual int patch(const char* aURLPath,
                    const char* aContentType,
                    int aContentLength,
                    const byte aBody[]) = 0;

  /** Connect to the server and start to send a DELETE request.
    @param aURLPath     Url to request
    @return 0 if successful, else error
  */
  virtual int del(const char* aURLPath) = 0;
  virtual int del(const String& aURLPath) = 0;

  /** Connect to the server and send a DELETE request
      with body and content type
    @param aURLPath     Url to request
    @param aContentType Content type of request body
    @param aBody        Body of the request
    @return 0 if successful, else error
  */
  virtual int del(const char* aURLPath,
                  const char* aContentType,
                  const char* aBody) = 0;
  virtual int del(const String& aURLPath,
                  const String& aContentType,
                  const String& aBody) = 0;
  virtual int del(const char* aURLPath,
                  const char* aContentType,
                  int aContentLength,
                  const byte aBody[]) = 0;

  // ================================================================

  /** Check if a header is available to be read.
    Use readHeaderName() to read header name, and readHeaderValue() to
    read the header value
    MUST be called after responseStatusCode() and before contentLength()
  */
  //  virtual bool headerAvailable() = 0;

  /** Test whether the end of the body has been reached.
    Only works if the Content-Length header was returned by the server
    @return true if we are now at the end of the body, else false
  */
  virtual bool completed() = 0;

  /** Return the length of the body.
    Also skips response headers if they have not been read already
    MUST be called after responseStatusCode()
    @return Length of the body, in bytes, or kNoContentLengthHeader if no
    Content-Length header was returned by the server
  */
  virtual int contentLength() = 0;

  /** Return the response body as a String
    Also skips response headers if they have not been read already
    MUST be called after responseStatusCode()
    @return response body of request as a String
  */
  virtual String responseBody() = 0;

  /** Get the HTTP status code contained in the response.
    For example, 200 for successful request, 404 for file not found, etc.
  */
  virtual int responseStatusCode() = 0;

  /** Connect to the server and start to send the request.
      If a body is provided, the entire request (including headers and body)
    will be sent
    @param aURLPath        Url to request
    @param aHttpMethod     Type of HTTP request to make, e.g. "GET", "POST",
    etc.
    @param aContentType    Content type of request body (optional)
    @param aContentLength  Length of request body (optional)
    @param aBody           Body of request (optional)
    @return 0 if successful, else error
  */
  virtual int startRequest(const char url_path[],
                           const char http_method[],
                           const char content_type[] = NULL,
                           int content_length = -1,
                           const byte body[] = NULL) = 0;

  // From Client
  virtual uint8_t connected() = 0;
  virtual uint32_t httpResponseTimeout() = 0;
  virtual void setHttpResponseTimeout(uint32_t timeout) = 0;
  virtual void stop() = 0;
  virtual operator bool() = 0;

  // TLS
  virtual bool setClientCA(const char certificate[]) = 0;
  virtual bool setClientPrivateKey(const char certificate[]) = 0;
  virtual bool setRootCA(const char certificate[]) = 0;

 protected:
  /** Reset internal state data back to the "just initialised" state
   */
  virtual void resetState() = 0;
};

#endif
