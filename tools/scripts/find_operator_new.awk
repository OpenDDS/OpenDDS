/^\/.*:[1-9][0-9]*/ {
    location = $0;
    next;
}

/<operator new/ && /call/ {
    print location;
}
