import os

def consolidate_c_files(source_folder, output_filename):
    # Ensure the output file is empty before starting
    with open(output_filename, 'w', encoding='utf-8') as outfile:
        # Loop through files in the directory
        for filename in sorted(os.listdir(source_folder)):
            if filename.endswith('.c'):
                file_path = os.path.join(source_folder, filename)
                
                # Write the header
                outfile.write(f"### {filename}\n")
                
                # Read the .c file and write its contents
                try:
                    with open(file_path, 'r', encoding='utf-8') as infile:
                        outfile.write(infile.read())
                except Exception as e:
                    outfile.write(f"// Error reading file: {e}")
                
                # Add extra newlines for separation
                outfile.write("\n\n")

    print(f"Successfully created {output_filename}")

if __name__=="__main__":
    folder_path = '.'
    output_file = '!consolidated_code.txt'

    consolidate_c_files(folder_path, output_file)