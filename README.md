# raw2header

### Takes in any file, spits out a header file.

The first parameter can be -16 to generate a 16 bit per entry little-endian header file or -b16 for the same in big-endian.
Then you specify the input filename, the output filename, and lastly the variable name you want in your header file.

Take note that if you have an odd number of bytes, it'll complain.  I am considering it offering a 0 to even up the data as an option.

This builds on mac, or Linux for certain.  It should build on a Windows PC, but I don't have Windows on *any* of my computers.
