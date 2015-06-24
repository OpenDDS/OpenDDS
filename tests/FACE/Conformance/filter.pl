#!/usr/bin/perl -p
s/Assertion/Assertn/g if /DEBUG:/;
s/APEX_ERROR/APEX_ERR/g;
s/TAOLIB_ERROR/TAOLIB_ERR/g;
s/TAOLIB_DEBUG/TAOLIB_DBG/g;
