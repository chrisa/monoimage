
/*
 * monoboot
 *
 * chooses a monoimage to boot. 
 *
 * looks at the boot config file for the preferred image, then at the boot log
 * to decide if we should go for the fallback image. then kexec-load the image,
 * log our decision, make sure we're mounted read-only and then kexec the image
 * 
 */

