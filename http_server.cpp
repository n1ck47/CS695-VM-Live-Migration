#include "http_server.hh"

#include <vector>

#include <sys/stat.h>

#include <fstream>
#include <sstream>

vector<string> split(const string &s, char delim) {
  vector<string> elems;

  stringstream ss(s);
  string item;

  while (getline(ss, item, delim)) {
    if (!item.empty())
      elems.push_back(item);
  }

  return elems;
}

HTTP_Request::HTTP_Request(string request) {
  vector<string> lines = split(request, '\n');
  vector<string> first_line = split(lines[0], ' ');

  this->HTTP_version = "1.0"; // We'll be using 1.0 irrespective of the request

  /*
   TODO : extract the request method and URL from first_line here
  */

  this->method = first_line[0];
  this->url = first_line[1];

  if (this->method != "GET") {
    cerr << "Method '" << this->method << "' not supported" << endl;
    exit(1);
  }
}

HTTP_Response *handle_request(string req) {

  HTTP_Request *request = new HTTP_Request(req);

  HTTP_Response *response = new HTTP_Response();

  string url = string("html_files") + request->url;

  response->HTTP_version = "1.0";
  response->content_type = "text/html";

  struct stat sb;

  bool flag = 0;
  if (stat(url.c_str(), &sb) == 0) // requested path exists
  {
    string body;
    ifstream fs;
    if(S_ISDIR(sb.st_mode)) {
      /*
      In this case, requested path is a directory.
      TODO : find the index.html file in that directory (modify the url
      accordingly)
      */
      url = url + "/index.html";
    }
    fs.open(url);
    if(fs){
      response->status_code = "200";
      response->status_text = "OK";

      while(getline(fs,body)){
        response->body = response->body + body + "\n" ;
      }

      fs.close();
      fs.open(url, ios::ate);
      response->content_length = to_string(fs.tellg());
      fs.close();
    }
    else{
      flag = 1;
    }
  
  }

  else {
    flag = 1;
  }

  if(flag){
    response->status_code = "404";
    response->status_text = "NOT FOUND";
    response->body = "ERROR NOT FOUND";
    response->content_length = "15";
  }

  delete request;

  return response;
}

string HTTP_Response::get_string() {
  
  string resString = "HTTP/" + this->HTTP_version + " " + this->status_code + " " + this->status_text + "\n";
  resString += "Content-Type: " + this->content_type + "\n";
  resString += "Content-Length: " + this->content_length + "\n";
  resString += "Connection: Closed\n\n";
  resString += this->body;



  return resString;
}
