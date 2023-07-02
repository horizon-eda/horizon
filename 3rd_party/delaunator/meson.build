foreach pic : [true, false]
    delaunator_t = static_library('delaunator' + (pic?'-pic':''), files('delaunator.cpp'), pic:pic)
    if pic
        delaunator_pic = delaunator_t
    else
        delaunator_nopic = delaunator_t
    endif
endforeach
