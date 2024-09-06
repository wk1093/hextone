module hexagon(radius) {
    circle(r=radius,$fn=6);
}

// we want a tiling grid of 2 x 4 hexagons
rad = 12;
xgrid = 2;
ygrid = 4;
spacing = 5; // space in between hexagons
sqarehole = 14;
keycap_height = 8;
keycap_thickness=1.5;
keycap_stemrad=3.5;
cross_depth=4;
cross_length = 4.4;
horizontal_cross_width = 1.4;
vertical_cross_width = 1.3;
extra_outer_cross_width = 2.10;
extra_outer_cross_height = 1.0;
extra_vertical_cross_length = 1.1;
holediff = -0.5;
sq_width = 2;
hex_width = 2;
sq_off = -10;
key_off = 10;

// 0 = all, 1 = hexgrid, 2 = squaregrid, 3 = keygrid, 4 = single key
display = 0;

fac = 1.75;
border = spacing / 1.5;
// the y grid is straight up and down
// the x grid is offset because it's a hexagon
// the final shape will be like a rhombus or something

module keycap() {
    // make a single keycap
    difference() {
        linear_extrude(keycap_height) {
            hexagon(rad);
        }
        translate([0,0,-.01]) {
            linear_extrude(keycap_height-keycap_thickness+.01) {
                hexagon(rad-keycap_thickness);
            }
        }
    }
    difference() {
        translate([0,0,(keycap_height)/2]) {
            cube([(cross_length+extra_outer_cross_width),
                    (cross_length+extra_outer_cross_height),keycap_height], center=true);
        }
        translate([0,0,(cross_depth)/2-.01]) {
            cube([cross_length, horizontal_cross_width, cross_depth+.01], center=true);
            cube([vertical_cross_width,cross_length+extra_vertical_cross_length,cross_depth], center=true );
        }
    }

}

// make a grid of the hexagons
module hexagongrid() {
    for (i = [0:xgrid-1]) {
        for (j = [0:ygrid-1]) {
            hrad = rad - holediff;
            ytranslate = j * (rad*fac + spacing) - i * (rad*fac + spacing) * 0.5;
            // the x spacing affects the y spacing, because it is at an angle of 60 degrees
            xtranslate = i * (rad*fac + spacing) * 0.866025403784;
            translate([xtranslate, ytranslate, 0]) {
                hexagon(hrad);
            }
        }
    }
}

module squaregrid() {
    for (i = [0:xgrid-1]) {
        for (j = [0:ygrid-1]) {
            ytranslate = j * (rad*fac + spacing) - i * (rad*fac + spacing) * 0.5;
            // the x spacing affects the y spacing, because it is at an angle of 60 degrees
            xtranslate = i * (rad*fac + spacing) * 0.866025403784;
            translate([xtranslate-sqarehole/2, ytranslate-sqarehole/2, 0]) {
                square(sqarehole);
            }
        }
    }
}

module keygrid() {
    for (i = [0:xgrid-1]) {
        for (j = [0:ygrid-1]) {
            ytranslate = j * (rad*fac + spacing) - i * (rad*fac + spacing) * 0.5;
            // the x spacing affects the y spacing, because it is at an angle of 60 degrees
            xtranslate = i * (rad*fac + spacing) * 0.866025403784;
            translate([xtranslate, ytranslate, 0]) {
                keycap();
            }
        }
    }
}

module outline() {
    for (i = [0:xgrid-1]) {
        for (j = [0:ygrid-1]) {
            brad = rad + border;
            ytranslate = j * (rad*fac + spacing) - i * (rad*fac + spacing) * 0.5;
            // the x spacing affects the y spacing, because it is at an angle of 60 degrees
            xtranslate = i * (rad*fac + spacing) * 0.866025403784;
            translate([xtranslate, ytranslate, 0]) {
                hexagon(brad);
            }
        }
    }
}

module stuff() {
    if (display == 0 || display == 1) {
        linear_extrude(hex_width) {
            difference() {
                outline();
                hexagongrid();
            }
        }
    }
    if (display == 0 || display == 2) {
        translate([0, 0, sq_off]) {
            linear_extrude(sq_width) {
                difference() {
                    outline();
                    squaregrid();
                }
            }
        }
    }
    if (display == 0 || display == 3) {
        translate([0, 0, key_off]) {
            keygrid();
        }
    }

    if (display == 4) {
        keycap();
    }
}
/*
stuff();
translate([46,0,0]) {
    stuff();
}
translate([0.5,105,0]) {
    stuff();
}
translate([46.5, 105, 0]) {
    stuff();
}
*/
xl = 0;
yl = 0;

for (i = [0:xl]) {
    for (j = [0:yl]) {
        translate([i*46+0.5*j, 105*j, 0]) {
            stuff();
        }
    }
}


