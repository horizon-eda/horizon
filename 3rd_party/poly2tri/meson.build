foreach pic : [true, false]
    poly2tri_t = static_library('poly2tri' + (pic?'-pic':''), files(
        'common/shapes.cpp',
        'sweep/cdt.cpp',
        'sweep/sweep.cpp',
        'sweep/sweep_context.cpp',
        'sweep/advancing_front.cpp',
    ), pic:pic)
    if pic
        poly2tri_pic = poly2tri_t
    else
        poly2tri_nopic = poly2tri_t
    endif
endforeach
