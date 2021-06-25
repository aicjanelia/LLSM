run("Open...");
title=getTitle();
path=getDirectory("Image");
Dialog.create("Cropping Parameters");
Dialog.addNumber("X-Y FOV (odd number)", 25);
Dialog.addNumber("Z FOV (odd number", 25);
Dialog.show()
xcrop=Dialog.getNumber();
zcrop=Dialog.getNumber();
if (xcrop%2==1) {
	xcrop = xcrop-1;
}
if (zcrop%2==1) {
	zcrop=zcrop-1;
}
xcrop=xcrop/2;
zcrop=zcrop/2;
run("Z Project...", "projection=[Max Intensity]");
run("Find Maxima...", "prominence=100 exclude output=[Point Selection]");
run("Set Measurements...", "centroid redirect=None decimal=3");
run("Measure");
close();
x=getResult("X", 0);
y=getResult("Y", 0);
selectWindow(title);
run("Reslice [/]...", "output=1.000 start=Top");
run("Z Project...", "projection=[Max Intensity]");
run("Find Maxima...", "prominence=100 exclude output=[Point Selection]");
run("Measure");
z=getResult("Y", 1);
close();
close();
selectWindow(title);
makeRectangle(x-xcrop, y-xcrop, xcrop*2+1, xcrop*2+1);
run("Crop");
z1=toString(z-zcrop);
z2=toString(z+zcrop);
run("Make Substack...", "  slices="+z1+"-"+z2);
run("Subtract...", "value=100 stack");
saveAs("Tiff", path + "cropped_"+title);
run("Clear Results");
close();
close();
selectWindow("Results"); 
run("Close" );