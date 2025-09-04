//
//  font_manager.hpp
//  TechDemo 1
//
//  Created by Sigurd T Seteklev on 25/01/16.
//  Copyright © 2016 Sigurd T Seteklev. All rights reserved.
//

#ifndef font_manager_hpp
#define font_manager_hpp

#include <stdio.h>
#include "base/math/Math.h"

#include <vector>
#include <map>
struct  glyph {
    // Rendering position
    int             startX, startY;
    unsigned int    width, height;
    
    // Bitmap location
    int             bitmapTop, bitmapLeft;
    
    int             advance;
};

struct kerning {
    char    first;
    char    second;
    int     kerning;
};

struct font {
    unsigned int            textureId;
    std::map< char, glyph>  glyphs;
    
    unsigned int            lineSpacing;
    
    std::vector<kerning>    kernings;
};

bool    createFont( const char *fontname, unsigned int size, float spacing, font *fnt);
bool    renderText( const font &fnt, const math::Vec2f &pos, const float scale, const char *text );

void    renderTextBox( const font &fnt, const math::Vec2f &pos, const float scale, const char *text );


#endif /* font_manager_hpp */
