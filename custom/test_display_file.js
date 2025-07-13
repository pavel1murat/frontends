    // Example using Fetch API in client-side JavaScript
    async function displayServerFile(filename) {
        try {
            const response = await fetch(`/getFileContent/${filename}`);
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            const fileContent = await response.text();
            document.getElementById('fileDisplayArea').innerText = fileContent; // Display in a designated HTML element
        } catch (error) {
            console.error('Error fetching file:', error);
        }
    }

    // Call this function with the desired filename, e.g., on a button click
    // displayServerFile('myTextFile.txt');
