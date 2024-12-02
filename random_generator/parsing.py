import mmap
from concurrent.futures import ThreadPoolExecutor

def parse_fasta_and_save(fasta_file, output_prefix, chunk_size=10_000_000):
    """
    Optimized FASTA parsing: Removes 'N', processes in chunks, and writes using parallel threads.
    
    :param fasta_file: Input FASTA file path
    :param output_prefix: Prefix for output text files
    :param chunk_size: Number of characters per output file
    """
    def write_chunk(sequence, chunk_index):
        """Writes a single chunk to a file."""
        output_file = f"{output_prefix}_{chunk_index}.txt"
        with open(output_file, "w") as out:
            out.write(sequence)
        print(f"Saved {output_file}")

    current_sequence = []
    current_length = 0
    chunk_index = 0
    total_processed = 0

    # Thread pool for writing chunks
    with ThreadPoolExecutor() as executor:
        futures = []

        with open(fasta_file, "r") as file:
            # Use mmap for efficient reading
            with mmap.mmap(file.fileno(), length=0, access=mmap.ACCESS_READ) as mm:
                for line in iter(mm.readline, b""):
                    line = line.decode("utf-8").strip()
                    if line.startswith(">"):
                        continue  # Skip headers
                    
                    # Clean and append the line
                    cleaned_line = line.replace("N", "")
                    current_sequence.append(cleaned_line)
                    current_length += len(cleaned_line)
                    total_processed += len(cleaned_line)

                    # If chunk size is reached, submit write task
                    while current_length >= chunk_size:  # Process multiple chunks if needed
                        full_sequence = "".join(current_sequence)
                        future = executor.submit(write_chunk, full_sequence[:chunk_size], chunk_index)
                        futures.append(future)

                        # Update sequence and length
                        remaining_sequence = full_sequence[chunk_size:]
                        current_sequence = [remaining_sequence]
                        current_length = len(remaining_sequence)
                        chunk_index += 1

                        # Print progress
                        print(f"Processing... Total characters processed: {total_processed}", end="\r")

        # Write remaining data
        if current_sequence:
            future = executor.submit(write_chunk, "".join(current_sequence), chunk_index)
            futures.append(future)

        # Ensure all write tasks are complete
        for future in futures:
            future.result()

    print(f"\nFinished processing. Total characters processed: {total_processed}")


parse_fasta_and_save("/Users/a1/algorithm/Algorithm/random_generator/modified_genome.txt", "out_put_100_reads", chunk_size=100)
