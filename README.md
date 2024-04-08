# File Fortress

File Fortress is a web-based file management system designed for storing, retrieving, and managing files on a server. This project provides a user-friendly web interface for interacting with the server, including uploading, downloading, requesting, and deleting files. Additionally, it offers a directory page that allows users to view and search for files stored on the server.

![Home page](![Home](https://github.com/Surajsj1534/Gateway-Device/assets/124913477/86a7b386-e955-484b-9aaa-df64f95f3d72)


## Features

- **Upload Files**: Users can easily upload files to the server through the web interface.
- **Download Files**: Download files stored on the server to your local machine.
- **Request Files**: Submit requests for specific files from the server.
- **Delete Files**: Delete files from the server through the web interface.
- **View Directory**: View a list of available files in the directory, including search functionality.
- **User-Friendly Interface**: The project features a user-friendly, responsive design for easy navigation.

## Project Structure

- `main.ino`: This Arduino sketch file contains the server-side logic. It handles file storage, requests, and deletion. Make sure to upload this sketch to your ESP32 board.
- `web`: This folder contains all the static HTML files for the web interface.
- `files`: This folder contains the initial files that are fed into the master ESP32.

## Setup and Usage

1. Clone this repository to your local machine or upload the HTML files to a web server.
2. Ensure you have an ESP32 board set up and connected to your network.
3. Upload the `main.ino` sketch to your ESP32 board using the Arduino IDE.
4. Open the web interface on a device connected to the same network as your board.
5. Use the provided links in the navigation bar to access different features, such as uploading, downloading, and managing files.
6. On the directory page, you can search for specific files by name to view and download them.
7. Ensure you're familiar with the features and functionalities mentioned above to efficiently manage your files.

## Dependencies

- [Arduino IDE](https://www.arduino.cc/en/software) for uploading the server-side code to your ESP32 board.

## Gateway Implementation

- The project now includes a master-slave architecture, similar to CDN systems, where the master ESP32 serves as the primary interface for client interactions.
- Clients interact with the master ESP32 for basic operations such as uploading, downloading, and deleting files.
- When a client requests a file that is not present on the master ESP32, the backend initiates a bridge connection between the master and slave ESP32s.
- The slave ESP32 acts as a database and checks if it has the requested file. If found, the slave sends the file's content to the master, which then renders it on the client-side frontend.
- If the file is not found on the slave, an appropriate error message is displayed on the frontend.

## Contributing

Contributions to this project are welcome. You can fork the repository, make improvements, and create a pull request.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgments

- The project utilizes the Arduino platform for server-side functionality.
- The web interface incorporates HTML, CSS, and JavaScript for user interaction.

## Contact

For any questions or support, please contact [Suraj Jadhav](surajjadhavsj777@gmail.com).

---

Happy file management with File Fortress!
