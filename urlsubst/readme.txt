Using Flash for Banner Ads

This URL substitution application can be used to implement click through tracking for Flash banner ads. The basic idea of click through tracking is that every url in an ad delivered to a given user should have a unqiue serial number embeded in the URL. These URLs typically call a CGI program which tracks the click and redirects the request to an appropriate URL. 

By replacing part or all of a URL in a .swf file with a URL that contains a unique serial number, this type of click through tracking can be implemented in Flash. The idea is that a .swf file is generator on the ad server for each ad request. The enclosed program can be used to create serialized URLs in these unique ads.

The urlsubst program should easy compile on any standard C++ compiler and can also be integrated into specific server software in source form. A makefile is provided for Microsoft Visual C++ 5.0.

The demo directory shows how this application can be used.