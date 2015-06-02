/^\/.*:[1-9][0-9]*/ {
    location = $0;
    next;
}

/<operator new/ && /(call)|(jmp)|(je)|(jne)|(jb)|(jbe)|(ja)|(jae)|(jl)|(jle)|(jg)|(jge)|(js)|(jns)/ {
    print location;
}
