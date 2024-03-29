include ../../Makefile.in.$(shell uname).SDL

PLATFORM=sdl

PLATFORMSOURCES=../../src/platform/$(PLATFORM)/agg_platform_support.o

CXXFLAGS= $(AGGCXXFLAGS) -I../../include \
-L../../src \
$(PIXFMT)

CXXFREETYPEFLAGS= $(AGGCXXFLAGS) -Wall \
-I../../include \
-I../../font_freetype \
-I/usr/local/include/freetype2 \
-L../../src \
$(PIXFMT)

LIBS = $(AGGLIBS) -lm

base:
	cd ../../src/; make
	make aa_demo
	make aa_test
	make alpha_gradient
	make alpha_mask
	make alpha_mask2
	make alpha_mask3
	make bspline
	make blur
	make blend_color
	make bezier_div
	make circles
	make component_rendering
	make conv_contour
	make conv_dash_marker
	make conv_stroke
	make gamma_correction
	make gamma_ctrl
	make gouraud
	make gradient_focal
	make gradients
	make graph_test
	make idea
	make lion
	make lion_lens
	make lion_outline
	make multi_clip
	make pattern_fill
	make perspective
	make polymorphic_renderer
	make raster_text
	make rasterizers
	make rasterizers2
	make rasterizer_compound
	make rounded_rect
	make scanline_boolean
	make scanline_boolean2
	make simple_blur
	make trans_polar
	make image_alpha
	make image_filters
	make image_filters2
	make image_fltr_graph
	make image_perspective
	make image_resample
	make image_transforms
	make image1
	make distortions
	make pattern_perspective
	make compositing
	make line_patterns
	make mol_view
	
freetype:
	make freetype_test
	make trans_curve1_ft
	make trans_curve2_ft

gpc:
	make gpc_test
	
all:
	make base
	make freetype
	make gpc

aa_demo: ../aa_demo.o $(PLATFORMSOURCES) 
	$(CXX) $(CXXFLAGS) $^ -o aa_demo $(LIBS)

aa_test: ../aa_test.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o aa_test $(LIBS)

alpha_gradient: ../alpha_gradient.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o alpha_gradient $(LIBS)

alpha_mask: ../alpha_mask.o ../parse_lion.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o alpha_mask $(LIBS)

alpha_mask2: ../alpha_mask2.o ../parse_lion.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o alpha_mask2 $(LIBS)

alpha_mask3: ../alpha_mask3.o ../make_arrows.o ../make_gb_poly.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o alpha_mask3 $(LIBS)
	
bezier_div: ../bezier_div.o ../interactive_polygon.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o bezier_div $(LIBS)

blur: ../blur.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o blur $(LIBS)

blend_color: ../blend_color.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o blend_color $(LIBS)

bspline: ../bspline.o ../interactive_polygon.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o bspline $(LIBS)

circles: ../circles.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o circles $(LIBS)

component_rendering: ../component_rendering.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o component_rendering $(LIBS)

compositing: ../compositing.o $(PLATFORMSOURCES) compositing.bmp
	$(CXX) $(CXXFLAGS) ../compositing.o $(PLATFORMSOURCES) -o compositing $(LIBS)

conv_contour: ../conv_contour.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o conv_contour $(LIBS)

conv_dash_marker: ../conv_dash_marker.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o conv_dash_marker $(LIBS)

conv_stroke: ../conv_stroke.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o conv_stroke $(LIBS)

distortions: ../distortions.o $(PLATFORMSOURCES) spheres.bmp
	$(CXX) $(CXXFLAGS) ../distortions.o $(PLATFORMSOURCES) -o distortions $(LIBS)

gamma_correction: ../gamma_correction.o $(PLATFORMSOURCES) 
	$(CXX) $(CXXFLAGS) $^ -o gamma_correction $(LIBS)

gamma_ctrl: ../gamma_ctrl.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o gamma_ctrl $(LIBS)

gouraud: ../gouraud.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o gouraud $(LIBS)

gradient_focal: ../gradient_focal.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o gradient_focal $(LIBS)

gradients: ../gradients.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o gradients $(LIBS)

graph_test: ../graph_test.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o graph_test $(LIBS)

idea: ../idea.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o idea $(LIBS)

image_alpha: ../image_alpha.o $(PLATFORMSOURCES) spheres.bmp
	$(CXX) $(CXXFLAGS) ../image_alpha.o $(PLATFORMSOURCES) -o image_alpha $(LIBS)

image_filters: ../image_filters.o $(PLATFORMSOURCES) spheres.bmp 
	$(CXX) $(CXXFLAGS) ../image_filters.o $(PLATFORMSOURCES) -o image_filters $(LIBS)

image_filters2: ../image_filters2.o $(PLATFORMSOURCES) spheres.bmp 
	$(CXX) $(CXXFLAGS) ../image_filters2.o $(PLATFORMSOURCES) -o image_filters2 $(LIBS)

image_fltr_graph: ../image_fltr_graph.o $(PLATFORMSOURCES) spheres.bmp 
	$(CXX) $(CXXFLAGS) ../image_fltr_graph.o $(PLATFORMSOURCES) -o image_fltr_graph $(LIBS)

image_perspective: ../image_perspective.o ../interactive_polygon.o $(PLATFORMSOURCES) spheres.bmp
	$(CXX) $(CXXFLAGS) ../image_perspective.o  ../interactive_polygon.o $(PLATFORMSOURCES) -o image_perspective $(LIBS)

image_resample: ../image_resample.o  ../interactive_polygon.o $(PLATFORMSOURCES) spheres.bmp
	$(CXX) $(CXXFLAGS) ../image_resample.o ../interactive_polygon.o $(PLATFORMSOURCES) -o image_resample $(LIBS)

image_transforms: ../image_transforms.o $(PLATFORMSOURCES) spheres.bmp
	$(CXX) $(CXXFLAGS) ../image_transforms.o $(PLATFORMSOURCES) -o image_transforms $(LIBS)

image1: ../image1.o $(PLATFORMSOURCES) spheres.bmp
	$(CXX) $(CXXFLAGS) ../image1.o $(PLATFORMSOURCES) -o image1 $(LIBS)

line_patterns: ../line_patterns.o $(PLATFORMSOURCES) 1.bmp 2.bmp 3.bmp 4.bmp 5.bmp 6.bmp 7.bmp 8.bmp 9.bmp
	$(CXX) $(CXXFLAGS) ../line_patterns.o $(PLATFORMSOURCES) -o line_patterns $(LIBS)

lion: ../lion.o ../parse_lion.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o lion $(LIBS)

lion_lens: ../lion_lens.o ../parse_lion.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o lion_lens $(LIBS)

lion_outline: ../lion_outline.o ../parse_lion.o $(PLATFORMSOURCES) 
	$(CXX) $(CXXFLAGS) $^ -o lion_outline $(LIBS)

mol_view: ../mol_view.o $(PLATFORMSOURCES) 1.sdf
	$(CXX) $(CXXFLAGS) ../mol_view.o $(PLATFORMSOURCES) -o mol_view $(LIBS)

multi_clip: ../multi_clip.o ../parse_lion.o $(PLATFORMSOURCES) 
	$(CXX) $(CXXFLAGS) $^ -o multi_clip $(LIBS)

pattern_fill: ../pattern_fill.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o pattern_fill $(LIBS)

pattern_perspective: ../pattern_perspective.o ../interactive_polygon.o $(PLATFORMSOURCES) agg.bmp
	$(CXX) $(CXXFLAGS) ../pattern_perspective.o ../interactive_polygon.o $(PLATFORMSOURCES) -o pattern_perspective $(LIBS)

perspective: ../perspective.o ../interactive_polygon.o ../parse_lion.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o perspective $(LIBS)

polymorphic_renderer: ../polymorphic_renderer.o $(PLATFORMSOURCES) 
	$(CXX) $(CXXFLAGS) $^ -o polymorphic_renderer $(LIBS)

raster_text: ../raster_text.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o raster_text $(LIBS)

rasterizers: ../rasterizers.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o rasterizers $(LIBS)

rasterizers2: ../rasterizers2.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o rasterizers2 $(LIBS)

rasterizer_compound: ../rasterizer_compound.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o rasterizer_compound $(LIBS)

rounded_rect: ../rounded_rect.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o rounded_rect $(LIBS)

scanline_boolean: ../scanline_boolean.o ../interactive_polygon.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o scanline_boolean $(LIBS)

scanline_boolean2: ../scanline_boolean2.o ../make_arrows.o ../make_gb_poly.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o scanline_boolean2 $(LIBS)

simple_blur: ../simple_blur.o ../parse_lion.o $(PLATFORMSOURCES) 
	$(CXX) $(CXXFLAGS) $^ -o simple_blur $(LIBS)

trans_polar: ../trans_polar.o $(PLATFORMSOURCES) 
	$(CXX) $(CXXFLAGS) $^ -o trans_polar $(LIBS)
	
	
freetype_test: ../freetype_test.o ../../font_freetype/agg_font_freetype.o $(PLATFORMSOURCES)  timesi.ttf
	$(CXX) $(CXXFREETYPEFLAGS) ../freetype_test.o ../../font_freetype/agg_font_freetype.o $(PLATFORMSOURCES) -o freetype_test $(LIBS) -lfreetype
	
trans_curve1_ft: ../trans_curve1_ft.o ../../font_freetype/agg_font_freetype.o  ../interactive_polygon.o $(PLATFORMSOURCES) timesi.ttf
	$(CXX) $(CXXFLAGS) ../trans_curve1_ft.o ../../font_freetype/agg_font_freetype.o  ../interactive_polygon.o $(PLATFORMSOURCES) -o trans_curve1_ft $(LIBS) -lfreetype
	
trans_curve2_ft: ../trans_curve2_ft.o ../../font_freetype/agg_font_freetype.o  ../interactive_polygon.o $(PLATFORMSOURCES) timesi.ttf
	$(CXX) $(CXXFLAGS) ../trans_curve2_ft.o ../../font_freetype/agg_font_freetype.o  ../interactive_polygon.o $(PLATFORMSOURCES) -o trans_curve2_ft $(LIBS) -lfreetype
	
gpc_test: ../gpc_test.o ../make_arrows.o ../make_gb_poly.o $(PLATFORMSOURCES)
	$(CXX) $(CXXFLAGS) $^ -o gpc_test $(LIBS)


clean:
	rm -f ../*.o
	rm -f ../../src/platform/$(PLATFORM)/agg_platform_support.o
	rm -f aa_demo
	rm -f aa_test
	rm -f alpha_gradient
	rm -f alpha_mask
	rm -f alpha_mask2
	rm -f alpha_mask3
	rm -f blur
	rm -f blend_color
	rm -f bezier_div
	rm -f bspline
	rm -f circles
	rm -f component_rendering
	rm -f conv_contour
	rm -f conv_dash_marker
	rm -f conv_stroke
	rm -f gamma_correction
	rm -f gamma_ctrl
	rm -f gouraud
	rm -f gradients
	rm -f gradient_focal
	rm -f graph_test
	rm -f idea
	rm -f lion
	rm -f lion_lens
	rm -f lion_outline
	rm -f multi_clip
	rm -f pattern_fill
	rm -f perspective
	rm -f polymorphic_renderer
	rm -f raster_text
	rm -f rasterizers
	rm -f rasterizers2
	rm -f rounded_rect
	rm -f scanline_boolean
	rm -f scanline_boolean2
	rm -f simple_blur
	rm -f trans_polar
	rm -f image_alpha
	rm -f image_filters
	rm -f image_filters2
	rm -f image_fltr_graph
	rm -f image_perspective
	rm -f image_resample
	rm -f image_transforms
	rm -f image1
	rm -f distortions
	rm -f pattern_perspective
	rm -f compositing
	rm -f line_patterns
	rm -f mol_view
	rm -f freetype_test
	rm -f trans_curve1_ft
	rm -f trans_curve2_ft
	rm -f gpc_test

agg.bmp:
	cp ../art/agg.bmp .

compositing.bmp:
	cp ../art/compositing.bmp .

spheres.bmp:
	cp ../art/spheres.bmp .

1.sdf:
	cp ../art/1.sdf .

1.bmp:
	cp ../art/line_patterns.bmp.zip .
	unzip -o line_patterns.bmp.zip

timesi.ttf:
	cp ../art/timesi.zip .
	unzip -o timesi.zip
	
../freetype_test.o:	../freetype_test.cpp
	@echo \< $*.cpp \>
	$(CXX) -c $(CXXFREETYPEFLAGS) $*.cpp -o $@
	
../trans_curve1_ft.o:	../trans_curve1_ft.cpp
	@echo \< $*.cpp \>
	$(CXX) -c $(CXXFREETYPEFLAGS) $*.cpp -o $@
	
../trans_curve2_ft.o:	../trans_curve2_ft.cpp
	@echo \< $*.cpp \>
	$(CXX) -c $(CXXFREETYPEFLAGS) $*.cpp -o $@
	
../../font_freetype/agg_font_freetype.o: ../../font_freetype/agg_font_freetype.cpp
	@echo \< $*.cpp \>
	$(CXX) -c $(CXXFREETYPEFLAGS) $*.cpp -o $@
	
../gpc_test.o:	../gpc_test.cpp
	@echo \< $*.cpp \>
	$(CXX) -c $(CXXFLAGS) -I../../gpc $*.cpp -o $@

%.o:	%.cpp
	@echo \< $*.cpp \>
	$(CXX) -c $(CXXFLAGS) $*.cpp -o $@

.PHONY : clean


