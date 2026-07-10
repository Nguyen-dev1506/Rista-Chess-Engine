import os
import glob
from PIL import Image

def process_image(img_path):
    img = Image.open(img_path).convert("RGBA")
    data = img.load()
    width, height = img.size

    visited = set()
    queue = []
    
    # Start flood fill from the borders
    for x in range(width):
        for y in (0, height - 1):
            queue.append((x, y))
            visited.add((x, y))
            
    for y in range(height):
        for x in (0, width - 1):
            if (x, y) not in visited:
                queue.append((x, y))
                visited.add((x, y))
                
    outside = set()
    
    while queue:
        x, y = queue.pop(0)
        
        r, g, b, a = data[x, y]
        avg = (r + g + b) / 3
        # Strict threshold to prevent leaking into white pieces
        if avg > 200:
            outside.add((x, y))
            for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                nx, ny = x + dx, y + dy
                if 0 <= nx < width and 0 <= ny < height:
                    if (nx, ny) not in visited:
                        visited.add((nx, ny))
                        queue.append((nx, ny))
                        
    # Apply transparency to outside pixels
    for x, y in outside:
        r, g, b, a = data[x, y]
        avg = (r + g + b) / 3
        if avg > 245:
            data[x, y] = (0, 0, 0, 0)
        else:
            # Convert anti-aliased edge to partial transparency black
            data[x, y] = (0, 0, 0, int(255 - avg))

    img.save(img_path)

dir_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "pngs")
for f in glob.glob(os.path.join(dir_path, "*.png")):
    print(f"Processing {os.path.basename(f)}")
    process_image(f)

print("Done")
