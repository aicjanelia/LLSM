// Macro to turn stacks of images into the average
// files need for the LLSM flatfield module

// Create a dialog box for the user to input the relevant files
Dialog.create("Files to Process");

Dialog.addMessage("Input one darkfield stack.");

Dialog.addFile("Darkfield Stack","");

Dialog.addMessage("");

Dialog.addMessage("Input one flatfield stack per channel.");
Dialog.addMessage("Channel entries without files will be ignored.");

Dialog.addMessage("");

Dialog.addNumber("Channel", 488);
Dialog.addFile("Flatfield Stack","");

Dialog.addMessage("");

Dialog.addNumber("Channel", 560);
Dialog.addFile("Flatfield Stack","");

Dialog.addMessage("");

Dialog.addNumber("Channel", 647);
Dialog.addFile("Flatfield Stack","");

Dialog.show();

dark_file = Dialog.getString();
ch1 = Dialog.getNumber();
ch1Stack = Dialog.getString();
ch2 = Dialog.getNumber();
ch2Stack = Dialog.getString();
ch3 = Dialog.getNumber();
ch3Stack = Dialog.getString();

imDir = File.getDirectory(dark_file);

// Create average darkfield stack
open(dark_file);
rename("DarkStack");
run("32-bit");
run("Z Project...", "projection=[Average Intensity]");
selectImage("DarkStack");
close();
selectImage("AVG_DarkStack");
saveAs("Tiff", imDir + "DarkAverage.tif");

print("Average Dark Image Saved.\n");

// Process each channel
// ch1
if (ch1Stack != ""){
	
	// Average flatfield
	open(ch1Stack);
	rename("FlatfieldStack" + ch1);
	run("32-bit");
	run("Z Project...", "projection=[Average Intensity]");
	selectImage("FlatfieldStack" + ch1);
	close();
	selectImage("AVG_FlatfieldStack" + ch1);
	saveAs("Tiff", imDir + "FlatfieldAverage" + ch1 + ".tif");

	print("Average Flatfield Image for " + ch1 + " Saved.\n");
	
	// Normalized Image
	imageCalculator("Subtract create 32-bit", "FlatfieldAverage" + ch1 + ".tif","DarkAverage.tif");
	selectImage("Result of " + "FlatfieldAverage" + ch1 + ".tif");
	getRawStatistics(dummy, mean, dummy, dummy, dummy, dummy2);
	run("Divide...", "value=" + mean);
	saveAs("Tiff", imDir + "I_N_" + ch1 + ".tif");

	print("Normalized Image for " + ch1 + " Saved.\n");
}

// ch2
if (ch2Stack != ""){
	
	// Average flatfield
	open(ch2Stack);
	rename("FlatfieldStack" + ch2);
	run("32-bit");
	run("Z Project...", "projection=[Average Intensity]");
	selectImage("FlatfieldStack" + ch2);
	close();
	selectImage("AVG_FlatfieldStack" + ch2);
	saveAs("Tiff", imDir + "FlatfieldAverage" + ch2 + ".tif");

	print("Average Flatfield Image for " + ch2 + " Saved.\n");
	
	// Normalized Image
	imageCalculator("Subtract create 32-bit", "FlatfieldAverage" + ch2 + ".tif","DarkAverage.tif");
	selectImage("Result of " + "FlatfieldAverage" + ch2 + ".tif");
	getRawStatistics(dummy, mean, dummy, dummy, dummy, dummy2);
	run("Divide...", "value=" + mean);
	saveAs("Tiff", imDir + "I_N_" + ch2 + ".tif");

	print("Normalized Image for " + ch2 + " Saved.\n");
}

// ch3
if (ch3Stack != ""){
	
	// Average flatfield
	open(ch3Stack);
	rename("FlatfieldStack" + ch3);
	run("32-bit");
	run("Z Project...", "projection=[Average Intensity]");
	selectImage("FlatfieldStack" + ch3);
	close();
	selectImage("AVG_FlatfieldStack" + ch3);
	saveAs("Tiff", imDir + "FlatfieldAverage" + ch3 + ".tif");

	print("Average Flatfield Image for " + ch3 + " Saved.\n");
	
	// Normalized Image
	imageCalculator("Subtract create 32-bit", "FlatfieldAverage" + ch3 + ".tif","DarkAverage.tif");
	selectImage("Result of " + "FlatfieldAverage" + ch3 + ".tif");
	getRawStatistics(dummy, mean, dummy, dummy, dummy, dummy2);
	run("Divide...", "value=" + mean);
	saveAs("Tiff", imDir + "I_N_" + ch3 + ".tif");

	print("Normalized Image for " + ch3 + " Saved.\n");
}

print("Saving complete");