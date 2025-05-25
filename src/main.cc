/*
 * Copyright (c) ghgltggamer 2025
 * Written by ghgltggamer
 * Licensed under the MIT License
 * Checkout the README.md for more information
 * GDIF -> Goblin's Demonic Image Flasher
*/

// This file is the file linking everything together

// C++ STL
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <fstream>
#include <cstdlib>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>

// Local headers
#include <htmlui/htmlui.hh>
#include <lblk/lblk.hh>
#include <ui/ui.hh>

void psystem(const std::string& command) {
    std::thread([command]() {
        system(command.c_str());
    }).detach();
}


std::string expandPath(const std::string& path) {
    if (!path.empty() && path[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home && std::string(home) != "/root") {
            // Normal user home
            return std::string(home) + path.substr(1);
        } else {
            // Running as root, try to get real user from USER env
            const char* user = std::getenv("USER");
            if (user && std::string(user) != "root") {
                // Assume home is /home/username
                return std::string("/home/") + user + path.substr(1);
            } else if (home) {
                // fallback to root home anyway
                return std::string(home) + path.substr(1);
            }
        }
    }
    return path;
}


std::string getRealUserHome() {
    // Get the UID of the original user before sudo
    const char* sudoUser = std::getenv("SUDO_USER");
    if (sudoUser) {
        struct passwd* pwd = getpwnam(sudoUser);
        if (pwd) {
            return std::string(pwd->pw_dir);
        }
    }
    // fallback to current HOME
    const char* home = std::getenv("HOME");
    return home ? std::string(home) : std::string();
}

// Main function
int main(){
	std::system ("rm -rf progress.txt drive_list.txt"); // clear any progress
 
	// std::this_thread::sleep_for(std::chrono::seconds(10));

	std::string usb_name = "Linux",
				drive_path = "",
	            iso_path = "";

	HTMLUI Window ("GDIF - Goblin's Demonic Image Flasher", 900, 600);
	Window.loadHTML(gdif::gui_src);
	
	// get the drive list
	std::system ("lsblk > drive_list.txt");
	std::ifstream idrive_file ("drive_list.txt");
	if (!idrive_file.is_open()){
		std::cout << ("Error > drive_list.txt was not found or accessible in the current working directory.");
		Window.executeJS ("alert ('Error > drive_list.txt was not found or accessible in the current working directory.')");
		std::this_thread::sleep_for(std::chrono::seconds(3));
		std::exit ( 3 );
	}
	std::string temp , idrive_content;
	while (std::getline (idrive_file, temp)) idrive_content += temp + '\n';

	auto drives = lblk_parser::parse_lsblk_output(idrive_content);
 
	 std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Wait for DOM
    for (const auto& [name, size] : drives) {
        std::cout << "Drive: " << name << ", Size: " << size << '\n';
		Window.executeJS("addDrive('" + name + "','" + size + "')");
    }
	
	Window.registerFunction ("gdif_cc_set_usb_type", [&](std::string usb){
		usb_name = usb;
		std::cout << "set the usb to "<<usb <<"\n";
	});

	Window.registerFunction ("gdif_cc_set_iso_path", [&](std::string iso){
		iso_path = iso;
		std::cout << "set the iso to "<<iso <<"\n";
	});

	Window.registerFunction ("gdif_cc_set_drive_path", [&](std::string drive){
		drive_path = drive;
		std::cout << "set the drive to "<<drive <<"\n";
	});

	Window.registerFunction ("gdif_cc_install_gwoeusb", [&](std::string arg){
		std::system ("mkdir -p ~/.config/gdif");
		std::system (std::string("wget https://raw.githubusercontent.com/darkyboys/gdif/refs/heads/main/woeusb/gwoeusb.bash -O " + getRealUserHome() + "/.config/gdif/gwoeusb.bash").c_str());
		std::system (std::string("chmod +x " + getRealUserHome() + "/.config/gdif/gwoeusb.bash").c_str());
		std::system ("echo 0 > progress.txt");
		Window.executeJS ("alert ('I Have installed the GWoeUSB for you, Please click on start flashing again to start the flashing')");
	});
 
	Window.registerFunction ("gdif_cc_flash_drive", [&](std::string str){
		std::cout << "Building...\n";
		std::ifstream ifile (iso_path);
		if (!ifile.is_open()){
			Window.executeJS ("alert('ISO File Don\\'t exists')");
			std::cout << "ISO File doesn't exists.\n";
		}

		else {
			int progress = 0;

			std::thread l1([&](){
				while (1){
					std::this_thread::sleep_for(std::chrono::seconds(2));
					std::ifstream ifile ("progress.txt");
					if (ifile.is_open()){
						std::string temp;
						while (std::getline(ifile, temp)){
							Window.executeJS("setProgress(" + temp + ")");
							progress = std::stoi(temp);
						}
						if (progress == 100){
							Window.executeJS("alert ('Your Image was successfully flashed!')");
							Window.executeJS("is_making = false;");
							progress = 0;
							break;
						}
					}
					else {
						std::cout << "Can't keep the progress as progress.txt wasnot found!\n";
					}
				}
			});			
 
			if (usb_name == "linux"){
				l1.detach();
				const std::string drive = drive_path;
				const std::string iso = iso_path;
				std::cout << "Flashing For Linux\n";
				// psystem("y | mkfs.ext4 /dev/" + drive_path + "\necho 35 > progress.txt");
				std::string dd = "dd if=\"";
				dd += iso_path;
				dd += "\" of=/dev/" + drive + " bs=1M status=progress\necho 100 > progress.txt";
				// psystem(dd);
				Window.executeJS("is_making = true;");
				psystem("umount /dev/" + drive + "*\necho 10 > progress.txt\n"
						"y | mkfs.ext4 /dev/" + drive + "\necho 35 > progress.txt\n"
						"dd if=\"" + iso + "\" of=/dev/" + drive + " bs=1M status=progress\necho 100 > progress.txt");
			}
			else if (usb_name == "windows"){
				std::cout << "Flashing for windows\n";
				
				std::ifstream ifile (getRealUserHome() + ("/.config/gdif/gwoeusb.bash"));
				std::cout << "ifile.is_open() = " << ifile.is_open() << "\n";
				if (!ifile.is_open()){
					Window.executeJS ("alert ('A Required GWoeUSB Installation wasn\'t found.')");
					Window.executeJS ("install_GWoeUSB();");
				}
				l1.detach();

				if (ifile.is_open()) {
					std::cout << "Running!\n";
					Window.executeJS("is_making = true;");
					// Window.executeJS ("setProgress (50);");
					const std::string local_iso = iso_path; 
					const std::string local_drive = drive_path;
					std::string stdjs;

					std::thread([local_iso, local_drive, &stdjs]() {
						// std::thread([local_iso, local_drive]() {
						if (std::system(std::string("umount /dev/" + local_drive + "*\necho 50 > progress.txt\n" + getRealUserHome() + "/.config/gdif/gwoeusb.bash --device \"" + local_iso + "\" /dev/" + local_drive).c_str()) == 0){
							std::system("echo 100 > progress.txt");
						}
						else {
							stdjs = "alert ('There was an unknown error occured in the backend, Please refer the terminal logs for more information')";
						}
							// std::cout << "[CMD] /home/gtg/.config/gdif/gwoeusb.bash --device \"" + local_iso + "\" /dev/" + local_drive << "\n";
						// }).join();
					}).detach();
					Window.executeJS (stdjs);

					// std::cout << "/home/gtg/.config/gdif/gwoeusb.bash --device " + iso + " /dev/sda" + drive + "\necho 100 > progress.txt" <<"\n";
				}
			}
			else {
				Window.executeJS ("alert ('Invalid OS Type Detected!')");
			}
		}
	});
	Window.run ();

}
