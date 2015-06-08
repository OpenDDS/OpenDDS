/^\/.*:[1-9][0-9]*/ {
    location = $0;
    next;
}

/<operator new/ && /call/ && !/operator new\(unsigned long, void\*\)/ && !/operator new\[\]\(unsigned long, void\*\)/ {
    print location;
}

/<operator delete/ && /call/ && !/operator delete\(unsigned long, void\*\)/ && !/operator delete\[\]\(unsigned long, void\*\)/ {
    print location;
}
