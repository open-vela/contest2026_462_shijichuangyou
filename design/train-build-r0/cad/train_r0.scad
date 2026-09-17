// Shiji removable-core hand-pushed train, R0 engineering layout, units mm.
// NOT a production release. No fit, mesh or collision validation performed.
// core_proxy is a dummy gauge, NOT an electronics enclosure.
// Use drawings and the measurement gate before releasing full-size parts.
part = "assembly";
fit_confirmed = false;
core_w = 70;  // PROPOSED finished enclosure envelope, NOT measured PCB width.
core_h = 78;  // PROPOSED; must include the final protected battery pack.
core_t = 28;  // PROPOSED; ports and cable bends still need measurement.
gap = 0.4;    // Per-side clearance; choose using a process-matched coupon.
wall = 2.4;
body_l = max(150, core_w + 80);
body_w = max(90, core_t + 62);
body_h = core_h + 9;
axle_a = 30;
axle_b = body_l - 30;
core_x = (body_l - core_w)/2;
core_y = -body_w/2 + wall + gap;
core_z = 5;
chassis_z = 20; // Axle center is z=0; flange bottom is z=-16 off track.
body_z = 24;
roof_t = 3;
$fn = 64;

module rounded_box(l,w,h,r=4) {
  hull() for(x=[r,l-r], y=[r,w-r])
    translate([x,y,0]) cylinder(r=r,h=h);
}
module zhole(x,y,z,h,d) { translate([x,y,z]) cylinder(d=d,h=h); }
module hexnut(x,y,z,h=3) {
  translate([x,y,z]) cylinder(r=5.8/(2*cos(30)),h=h,$fn=6);
}
module chassis() {
  difference() {
    union() {
      translate([0,-body_w/2,chassis_z]) rounded_box(body_l,body_w,4);
      for(x=[axle_a,axle_b],y=[-20,20])
        translate([x-6,y-4,-4]) cube([12,8,24.2]);
    }
    for(x=[axle_a,axle_b])
      translate([x,-body_w/2-1,0]) rotate([-90,0,0])
        cylinder(d=3.3,h=body_w+2);
    for(x=[9,body_l-9],y=[-body_w/2+8,body_w/2-8])
      zhole(x,y,19,7,3.4);
  }
}
module body() {
  difference() {
    union() {
      difference() {
        translate([0,-body_w/2,0]) rounded_box(body_l,body_w,body_h);
        translate([wall,-body_w/2+wall,wall])
          rounded_box(body_l-2*wall,body_w-2*wall,body_h+1,2);
        // Side opening shows the closed core, not a claimed glass cutout.
        translate([core_x+3,-body_w/2-1,core_z+3])
          cube([core_w-6,wall+2,core_h-6]);
      }
      // Two side guides, rear stop and bottom shelf, open at the top.
      for(x=[core_x-gap-wall,core_x+core_w+gap])
        translate([x,-body_w/2+wall-0.1,wall])
          cube([wall,core_t+2*gap+wall,body_h-wall]);
      translate([core_x-gap-wall,core_y+core_t+gap,wall])
        cube([core_w+2*gap+2*wall,wall,body_h-wall]);
      translate([core_x-gap-wall,-body_w/2+wall-0.1,wall])
        cube([core_w+2*gap+2*wall,core_t+2*gap+wall,core_z-wall]);
      // Body-to-chassis bosses; nuts are inserted from inside before core.
      for(x=[9,body_l-9],y=[-body_w/2+8,body_w/2-8])
        translate([x,y,0]) cylinder(r=5,h=8);
      // Roof fastener bridges connected to end walls.
      translate([0,-6,body_h-10]) cube([14,12,10]);
      translate([body_l-14,-6,body_h-10]) cube([14,12,10]);
    }
    for(x=[9,body_l-9],y=[-body_w/2+8,body_w/2-8]) {
      zhole(x,y,-1,10,3.4); hexnut(x,y,5.2,3.8);
    }
    for(x=[9,body_l-9]) {
      zhole(x,0,body_h-11,12,3.4); hexnut(x,0,body_h-3,4);
    }
  }
}
module roof() {
  difference() {
    union() {
      translate([-2,-body_w/2-2,0]) rounded_box(body_l+4,body_w+4,roof_t,5);
      // Hard stops above CLOSED core case corners, never over bare glass.
      for(x=[core_x,core_x+core_w-5])
        translate([x,core_y+core_t/2-3,-3.5]) cube([5,6,3.6]);
    }
    for(x=[9,body_l-9]) zhole(x,0,-4,9,3.4);
  }
}
module wheel() {
  // Local axis z: flange z=0..2; tread z=2..10.
  difference() {
    union() { cylinder(d=32,h=2); translate([0,0,2]) cylinder(d=28,h=8); }
    translate([0,0,-1]) cylinder(d=3.2,h=12);
  }
}
module track() {
  translate([0,-42,0]) cube([150,84,4]);
  for(y=[-32,32]) translate([0,y-2.5,3.99]) cube([150,5,3.01]);
}
module station() {
  rounded_box(60,35,5,3);
  for(x=[10,47]) translate([x,16,5]) cube([3,3,20]);
  translate([7.5,15.5,23]) cube([45,3,25]);
}
module end_stop() { cube([10,84,12]); }
module fit_coupon() {
  difference() {
    cube([56,34,8]);
    for(i=[0:2]) {
      c=[0.3,0.4,0.5][i];
      translate([2+i*18,8,2]) cube([10+2*c,27,7]);
      translate([7+i*18,4,-1]) cylinder(d=[3.0,3.2,3.4][i],h=10);
    }
  }
  translate([0,41,0]) cube([10,20,4]);
}
module core_proxy() { cube([core_w,core_t,core_h]); }
module assembly() {
  color("DimGray") chassis();
  color("Ivory") translate([0,0,body_z]) body();
  color("DarkSeaGreen") translate([0,0,body_z+body_h]) roof();
  color([0.4,0.65,0.45,0.55]) translate([core_x,core_y,body_z+core_z]) core_proxy();
  for(x=[axle_a,axle_b]) {
    color("Silver") translate([x,-42,0]) rotate([-90,0,0]) cylinder(d=3,h=84);
    color("SlateGray") translate([x,26,0]) rotate([-90,0,0]) wheel();
    color("SlateGray") translate([x,-26,0]) rotate([90,0,0]) wheel();
  }
}

assert(core_w>40 && core_h>45 && core_t>10, "Core envelope invalid");
assert(gap>=0.2 && gap<=0.8, "Use an approved coupon clearance");
assert(part=="assembly" || part=="fit_coupon" || part=="core_proxy" || fit_confirmed,
       "HOLD: measure the hardware, approve coupon and drawings before full parts");
if(part=="assembly") assembly();
else if(part=="fit_coupon") fit_coupon();
else if(part=="core_proxy") core_proxy();
else if(part=="chassis") translate([0,0,4]) chassis();
else if(part=="body") body();
else if(part=="roof") translate([0,0,roof_t]) rotate([180,0,0]) roof();
else if(part=="wheel") wheel();
else if(part=="track") track();
else if(part=="station") station();
else if(part=="end_stop") end_stop();
else assert(false,"Unknown part");
