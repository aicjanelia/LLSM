run("Open...");
title=getTitle();
path=getDirectory("Image");
run("Z Project...", "projection=[Max Intensity]");
run("Find Maxima...", "prominence=100 output=[Point Selection]");
run("Set Measurements...", "centroid redirect=None decimal=3");
run("Measure");
close()
x=getResult("X", 0);
y=getResult("Y", 0);
selectWindow(title);
run("Reslice [/]...", "output=1.000 start=Top");
run("Z Project...", "projection=[Max Intensity]");
run("Find Maxima...", "prominence=100 output=[Point Selection]");
run("Measure");
z=getResult("Y", 1);
close()
close()
selectWindow(title);
makeRectangle(x-12, y-12, 25, 25);
run("Crop");
z1=toString(z-12);
z2=toString(z+12);
run("Make Substack...", "  slices="+z1+"-"+z2);
run("Subtract...", "value=100 stack");
saveAs("Tiff", path + "cropped_"+title);
run("Clear Results");
close();
close();
selectWindow("Results"); 
run("Close" );