//
//  font_manager.cpp
//  TechDemo 1
//
//  Created by Sigurd T Seteklev on 25/01/16.
//  Copyright © 2016 Sigurd T Seteklev. All rights reserved.
//

#include "font_manager.hpp"

#include <iostream>

// STB True type
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

#include "base/math/Math.h"

#include "opengl/gl.h"


bool    createFont( const char *fontname, unsigned int size, float spacing, font *fnt) {
    
    const int padding = 5;
    
    char ttfBuffer[1<<20];
    stbtt_fontinfo font;
    
    FILE * file = fopen(fontname, "rb");
    
    if(!file ) {
        std::cerr << "Failed to load font " << fontname << std::endl;
        return false;
    }
    
    if(!fread(ttfBuffer, 1, 1<<20, fopen(fontname, "rb"))) {
        std::cerr << "Failed to load font file " << fontname << std::endl;
        return false;
    }
    stbtt_InitFont(&font, (const unsigned char*)ttfBuffer, stbtt_GetFontOffsetForIndex((const unsigned char*)ttfBuffer,0));
    
    float s = stbtt_ScaleForPixelHeight(&font, size);
    
    int ascent, descent, linegap;
    
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &linegap );
    
    fnt->lineSpacing = s*(ascent-descent+linegap);
    
    const int atlasWidth = 16*(size+2*padding), atlasHeight = 16*(size+2*padding);
    unsigned char *fontAtlas = (unsigned char*)malloc( atlasWidth*atlasHeight );
    memset( fontAtlas, 0 , atlasWidth*atlasHeight );
    
    for( unsigned char ascii = 0; ascii < 255; ++ascii) {
        glyph newGlyph;
        
        int advanceWidth, leftSideBearing;
        stbtt_GetCodepointHMetrics(&font, ascii, &advanceWidth, &leftSideBearing);
        
        newGlyph.advance = s*advanceWidth;
        
        unsigned char *bitmap = stbtt_GetCodepointBitmap(&font, s, s, ascii, (int*)&newGlyph.width, (int*)&newGlyph.height, (int*)&newGlyph.startX, (int*)&newGlyph.startY);
        
        unsigned char *sdf = stbtt_GetCodepointSDF(&font, s, ascii, padding, 180, 180/5.0, (int*)&newGlyph.width, (int*)&newGlyph.height, (int*)&newGlyph.startX, (int*)&newGlyph.startY);
        
        
        if( newGlyph.width > size+2*padding || newGlyph.height > size+2*padding ) {
            std::cout << "Glyph " << ascii << " oversized, expected " << size+2*padding << " got " << newGlyph.width << ", " << newGlyph.height << std::endl;
            return false;
        };
        
        // Copy bitmap into font atlas
        newGlyph.bitmapTop = (int)(size+2*padding)*(ascii/16);
        newGlyph.bitmapLeft = (int)(size+2*padding)*(ascii%16);
        
        for(int y = 0; y < newGlyph.height; ++y) {
            memcpy( &fontAtlas[(newGlyph.bitmapTop+y)*atlasWidth+newGlyph.bitmapLeft], &sdf[y*newGlyph.width], newGlyph.width);
        }
        
        stbtt_FreeSDF(sdf, nullptr);
        stbtt_FreeBitmap(bitmap, nullptr);
        fnt->glyphs[ascii] = newGlyph;
    }
    // TODO: Read kernings
    for(unsigned char asc1=0; asc1 < 255; ++asc1) {
        for( unsigned char asc2=0; asc2 < 255; ++asc2) {
            int kern = stbtt_GetCodepointKernAdvance(&font, asc1, asc2);
            if( kern ) {
                kerning newKern;
                
                newKern.first = asc1;
                newKern.second = asc2;
                newKern.kerning = kern*s;
                fnt->kernings.push_back(newKern);
            }
        }
    }
    
    unsigned char *rgbaAtlas = (unsigned char*)malloc( atlasWidth*atlasHeight*4);
    
    for(int y = 0; y < atlasHeight; ++y) {
        for( int x = 0; x < atlasWidth; ++x) {
            rgbaAtlas[ (x+y*atlasWidth) * 4 + 0 ] = 255;
            rgbaAtlas[ (x+y*atlasWidth) * 4 + 1 ] = 255;
            rgbaAtlas[ (x+y*atlasWidth) * 4 + 2 ] = 255;
            rgbaAtlas[ (x+y*atlasWidth) * 4 + 3 ] = fontAtlas[ (x+y*atlasWidth) ];
        }
    }
    
    // Upload font texture to GPU
    glGenTextures( 1, &fnt->textureId);
    glBindTexture( GL_TEXTURE_RECTANGLE_ARB, fnt->textureId);
    
    glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
    
    glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8, atlasWidth, atlasHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaAtlas );
    
    free( fontAtlas );
    free( rgbaAtlas );
    
    return true;
}

bool    renderText( const font &fnt, const math::Vec2f &pos, const float scale, const char *text ) {
    
    glDisable ( GL_TEXTURE_2D );
    
    glEnable( GL_TEXTURE_RECTANGLE_ARB );
    
    glBindTexture( GL_TEXTURE_RECTANGLE_ARB, fnt.textureId);
    
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0.65f);
    glBlendFunc( GL_ONE, GL_ZERO);
    
    math::Vec2f     penPos = pos;
    
    for( const char *chr = text; *chr; ++chr ) {
        
        auto glyphIterator = fnt.glyphs.find( *chr );
        
        const glyph &glyph = glyphIterator->second;
        
        if(*chr == '\n') {
            penPos[0] = pos[0];
            penPos[1] += fnt.lineSpacing*scale;
        } else if (*chr == '\r') {
            penPos[0] = pos[0];
        } else if (*chr == '\t') {
            penPos[0] += 4*fnt.glyphs.find(' ')->second.advance*scale;
        } else {
        
            // Render quad with textured font
            glBegin(GL_QUADS);
            glColor4d(1,1,1,1.0);
        
            glTexCoord2f(glyph.bitmapLeft, glyph.bitmapTop+glyph.height);
            glVertex3f(penPos[0]+glyph.startX*scale,penPos[1]+(glyph.startY+glyph.height)*scale,0);
        
            glTexCoord2f(glyph.bitmapLeft, glyph.bitmapTop);
            glVertex3f(penPos[0]+glyph.startX*scale,penPos[1]+glyph.startY*scale,0);
        
            glTexCoord2f(glyph.bitmapLeft+glyph.width, glyph.bitmapTop);
            glVertex3f(penPos[0]+(glyph.startX+glyph.width)*scale,penPos[1]+glyph.startY*scale,0);
        
            glTexCoord2f(glyph.bitmapLeft+glyph.width, glyph.bitmapTop+glyph.height);
            glVertex3f(penPos[0]+(glyph.startX+glyph.width)*scale,penPos[1]+(glyph.startY+glyph.height)*scale,0);

            glEnd();
        
            penPos[0] += glyph.advance*scale;
        
            // Adjust with kerning.
            char next = *(chr+1);
            if( next ) {
                for(auto curIt = fnt.kernings.begin(); curIt != fnt.kernings.end(); ++curIt ) {
                    if( curIt->first == *chr && curIt->second == next ) {
                        penPos[0] += curIt->kerning*scale;
                        continue;
                    }
                }
            }
        }
    }
    
    glDisable( GL_TEXTURE_RECTANGLE_ARB );
    glDisable( GL_ALPHA_TEST );
    
    glEnable ( GL_TEXTURE_2D );
    
    return true;
}

void    renderTextBox( const font &fnt, const math::Vec2f &pos, const float scale, const char *text ) {
    unsigned numLines = 1;
    unsigned width = 0;
    unsigned curLineWidth = 0;
    for( const char *chr = text; *chr; ++chr) {
        if( *chr == '\n' ) {
            ++numLines;
            width = width > curLineWidth ? width : curLineWidth;
            curLineWidth = 0;
        } else {
            auto glyphIterator = fnt.glyphs.find( *chr );
            const glyph &glyph = glyphIterator->second;
            
            curLineWidth += glyph.advance*scale;
            
            // Adjust with kerning.
            char next = *(chr+1);
            if( next ) {
                for(auto curIt = fnt.kernings.begin(); curIt != fnt.kernings.end(); ++curIt ) {
                    if( curIt->first == *chr && curIt->second == next ) {
                        curLineWidth += curIt->kerning*scale;
                        continue;
                    }
                }
            }
        }
    };
    width = width > curLineWidth ? width : curLineWidth;
    
    unsigned height = numLines*fnt.lineSpacing*scale;
    
    glDisable( GL_TEXTURE_2D);
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_ALPHA_TEST );
    glEnable( GL_BLEND );
    glBegin( GL_QUADS );
    const unsigned paddingX = 10, paddingY = 0;
    const float depthSort = 1000;
    glColor4d(0.5,0.5,0.5,0.75);
    
    glVertex3f(pos[0]-paddingX, pos[1]-(fnt.lineSpacing*0.75)*scale-paddingY,depthSort);
    glVertex3f(pos[0]+width+paddingX, pos[1]-(fnt.lineSpacing*0.75)*scale-paddingY,depthSort);
    glVertex3f(pos[0]+width+paddingX, pos[1]-(fnt.lineSpacing*0.75)*scale+height+paddingY,depthSort);
    glVertex3f(pos[0]-paddingX, pos[1]-(fnt.lineSpacing*0.75)*scale+height+paddingY,depthSort);
    glEnd();
    
    renderText( fnt, pos, scale, text);
    
    glColor4d(1,1,1,1);
    glEnable( GL_TEXTURE_2D);
    glEnable( GL_DEPTH_TEST );
};
