// Macro to process an image of a single fluorescence bead
// such that it can be used as a PSF.
// The macro (1) finds the bead centroid with subpixel
// accuracy, (2) shifts the bead to the center, (3) crops
// the final file, and (4) optionally resamples the PSF.
//
// Plugin dependencies: "3D Objects Counter" & "BigStitcher"

/////////////////////////////////////////////////////////////////////////////////////

// User GUI
Dialog.create("PSF Processing");
Dialog.addFile("Bead Image","");
Dialog.addNumber("Intensity Threshold for Peak Finding",1000);
Dialog.addMessage("Resampling is required for deconvolution prior to deskewing.");
Dialog.addCheckbox("Resample in Z?", false);
Dialog.addChoice("Microscope (Angle)",newArray("MOSAIC (-32.45)","LLSM (31.8)","Objective Scanned"));
Dialog.addNumber("PSF Z Step (um)",0.1);
Dialog.addNumber("Experiment Stage Step (um)",0);
Dialog.addMessage("The following parameters do not usually need to be changed.");
Dialog.addNumber("X-Y FOV (odd number)", 25);
Dialog.addNumber("Z FOV (odd number", 25);
Dialog.addNumber("Background Subtraction", 100);
Dialog.show();

// Grab the user inputs
bead_file = Dialog.getString();
imDir = File.getDirectory(bead_file);
imName = File.getName(bead_file);
intThresh = Dialog.getNumber();
resampleToggle = Dialog.getCheckbox();
microscope = Dialog.getChoice();
psfStep = Dialog.getNumber();
stepSize = Dialog.getNumber();
xcrop = Dialog.getNumber();
zcrop = Dialog.getNumber();
backSub = Dialog.getNumber();

// Check the user inputs
// Check resampling
if (resampleToggle){
	if (psfStep== 0){
		exit("Resampling requested but invalid PSF step size provided. Please provide the PSF step size.");
	}
	if (stepSize == 0){
		exit("Resampling requested but invalid stage step size provided. Please provide the stage step size.");
	}
}

if (microscope=="MOSAIC (-32.45)"){
	ang = 32.45*PI/180;
} else if (microscope=="LLSM (31.8)"){
	ang = (180-31.8)*PI/180;
} else if (microscope=="Objective Scanned"){
	ang = PI/2;
} else {
	exit("Invalid microscope selection.")
}

// Make sure the final PSF size is odd (so the bead can be centered)
if (xcrop%2==1) {
	xcrop = xcrop-1;
}
if (zcrop%2==1) {
	zcrop=zcrop-1;
}
xcrop=xcrop/2;
zcrop=zcrop/2;

// Open the bead file to work with
open(bead_file);
imName = getTitle();

/////////////////////////////////////////////////////////////////////////////////////

// Find the centroid
run("3D OC Options", "centroid show_masked_image_(redirection_requiered) dots_size=5 font_size=10 redirect_to=none");
run("3D Objects Counter", "threshold=" + intThresh + " slice=50 min.=10 max.=1000 exclude_objects_on_edges statistics");
close();
x=getResult("X", 0);
y=getResult("Y", 0);
z=getResult("Z", 0);
selectWindow("Results"); 
run("Close");

// Create a BigStitcher Dataset
run("Define Multi-View Dataset", "define_dataset=[Automatic Loader (Bioformats based)] project_filename=dataset.xml path=[" + bead_file + "] exclude=10 move_tiles_to_grid_(per_angle)?=[Move Tiles to Grid (interactive)] how_to_store_input_images=[Load raw data directly (no resaving)] load_raw_data_virtually metadata_save_path=[file:/" + imDir + "]");

run("Define Multi-View Dataset", "define_dataset=[Manual Loader (TIFF only, ImageJ Opener)] project_filename=dataset.xml multiple_timepoints=[NO (one time-point)] multiple_channels=[NO (one channel)] _____multiple_illumination_directions=[NO (one illumination direction)] multiple_angles=[NO (one angle)] multiple_tiles=[NO (one tile)] image_file_directory=[" + imDir +"] image_file_pattern=" + imName + " calibration_type=[Same voxel-size for all views] calibration_definition=[Load voxel-size(s) from file(s)]");

// Shift the bead to the center
run("Apply Transformations", "select=[file:/" + imDir + "dataset.xml] apply_to_angle=[All angles] apply_to_channel=[All channels] apply_to_illumination=[All illuminations] apply_to_tile=[All tiles] apply_to_timepoint=[All Timepoints] transformation=Affine apply=[Current view transformations (appends to current transforms)] timepoint_0_channel_0_illumination_0_angle_0=[1.0, 0.0, 0.0, -" + x + ", 0.0, 1.0, 0.0, -" + y + ", 0.0, 0.0, 1.0, -" + z-1 + "]"); // z-1 because of image indexing starts at 1 but I think BigStitcher starts at zero

// Define the cropping
run("Define Bounding Box", "select=[file:/" + imDir + "dataset.xml] process_angle=[All angles] process_channel=[All channels] process_illumination=[All illuminations] process_tile=[All tiles] process_timepoint=[All Timepoints] bounding_box=[Maximal Bounding Box spanning all transformed views] bounding_box_name=Crop minimal_x=-" + xcrop + " minimal_y=-" + xcrop + " minimal_z=-" + zcrop + " maximal_x=" + xcrop + " maximal_y=" + xcrop + " maximal_z=" + zcrop);

// Save the cropped PSF
// Note fusing to ImageJ and then saving to control file naming, etc. better
run("Image Fusion", "select=[file:/" + imDir + "dataset.xml] process_angle=[All angles] process_channel=[All channels] process_illumination=[All illuminations] process_tile=[All tiles] process_timepoint=[All Timepoints] bounding_box=Crop downsampling=1 interpolation=[Linear Interpolation] fusion_type=[Avg, Blending] pixel_type=[16-bit unsigned integer] interest_points_for_non_rigid=[-= Disable Non-Rigid =-] produce=[Each timepoint & channel] fused_image=[Display using ImageJ] define_input=[Auto-load from input data (values shown below)] display=[precomputed (fast, complete copy in memory before display)] min_intensity=0 max_intensity=1000");

run("Subtract...", "value=" + backSub + " stack"); // background subtraction
saveAs("Tiff", imDir + "cropped_"+ imName);

/////////////////////////////////////////////////////////////////////////////////////
// Run resampling if requested
if (resampleToggle){
	
	// Calculate rescaling factor
	scale = psfStep/stepSize/sin(ang);
	zcrop2 = zcrop*scale;
	
	// Affine transformation in z
	run("Apply Transformations", "select=[file:/" + imDir + "dataset.xml] apply_to_angle=[All angles] apply_to_channel=[All channels] apply_to_illumination=[All illuminations] apply_to_tile=[All tiles] apply_to_timepoint=[All Timepoints] transformation=Affine apply=[Current view transformations (appends to current transforms)] timepoint_0_channel_0_illumination_0_angle_0=[1.0, 0.0, 0.0,0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, " + scale + ", 0.0]");
	
	// Define the cropping
	run("Define Bounding Box", "select=[file:/" + imDir + "dataset.xml] process_angle=[All angles] process_channel=[All channels] process_illumination=[All illuminations] process_tile=[All tiles] process_timepoint=[All Timepoints] bounding_box=[Maximal Bounding Box spanning all transformed views] bounding_box_name=CropResample minimal_x=-" + xcrop + " minimal_y=-" + xcrop + " minimal_z=-" + zcrop*scale + " maximal_x=" + xcrop + " maximal_y=" + xcrop + " maximal_z=" + zcrop*scale);
	
	// Save the cropped PSF
	// Note fusing to ImageJ and then saving to control file naming, etc. better
	run("Image Fusion", "select=[file:/" + imDir + "dataset.xml] process_angle=[All angles] process_channel=[All channels] process_illumination=[All illuminations] process_tile=[All tiles] process_timepoint=[All Timepoints] bounding_box=Crop downsampling=1 interpolation=[Linear Interpolation] fusion_type=[Avg, Blending] pixel_type=[16-bit unsigned integer] interest_points_for_non_rigid=[-= Disable Non-Rigid =-] produce=[Each timepoint & channel] fused_image=[Display using ImageJ] define_input=[Auto-load from input data (values shown below)] display=[precomputed (fast, complete copy in memory before display)] min_intensity=0 max_intensity=1000");
	
	run("Set Scale...", "distance=1 known=1 unit=pixels"); // don't need scaling
	run("Subtract...", "value=" + backSub + " stack"); // background subtraction
	saveAs("Tiff", imDir + "cropped_resampled_"+ imName);
	
}
