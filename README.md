
# Email Recognition with OpenCV

### Problem
Given an image of a business card, extract the email address printed on it.

### Solution

Start with a photo with card edges visible:

![Source image](https://i.imgur.com/soRWIMK.jpg "Source image")

Using the Canny detector, find contours:

![Contours](https://i.imgur.com/rU3oc6A.png "Contours")

Choose the best contour, crop the image to contain only its bounding box (possibly rotated):

![Card Isolation](https://i.imgur.com/4L7NKf2.png "Card Isolation")

Using morphological operations, find what looks like text fields and isolate the results:

![Field Detection](https://i.imgur.com/CzfOdXi.png "Field Detection")

Perform OCR on every detected field, obtaining their text representations. Finally, select the best text based on its similarity to an e-mail address. In the interactive mode, simply recognise the text in the current field instead.

![Field Examination](https://i.imgur.com/Rn1E7cB.png "Field Examination")

### Dependencies

This project uses OpenCV 3, Tesseract and Leptonica. To install the latter libraries, you can simply get the packages `tesseract-ocr-dev libleptonica-dev` (in a Debian-based Linux).

### Command-line options

    ./convert [-cut | -text] filename [filenames...]
    
    -cut        Only perform card search. Outputs coordinates to stdout
    -text       No GUI. Outputs best guess to stdout
    filename    Source image. Supports multiple images
