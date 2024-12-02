from concurrent.futures import ThreadPoolExecutor
import os

def parse_and_save_chunk(chunk_data, chunk_index, output_prefix):
    """
    처리된 청크 데이터를 파일로 저장
    """
    output_file = f"{output_prefix}_{chunk_index}.txt"
    with open(output_file, "w") as f:
        f.write(chunk_data)
    print(f"Saved {output_file}")

def parse_fasta_multithread(fasta_file, output_prefix, chunk_size=10_000_000, num_threads=4):
    """
    FASTA 파일을 다중 스레드로 처리 및 저장
    """
    def process_chunk(chunk, chunk_index):
        # 'N' 제거 후 파일 저장
        clean_data = chunk.replace("N", "")
        parse_and_save_chunk(clean_data, chunk_index, output_prefix)

    file_size = os.path.getsize(fasta_file)
    num_chunks = (file_size // chunk_size) + 1

    # ThreadPoolExecutor 활용
    with ThreadPoolExecutor(max_workers=num_threads) as executor:
        futures = []
        with open(fasta_file, "r") as f:
            for chunk_index in range(num_chunks):
                chunk_data = f.read(chunk_size)  # 파일 청크 읽기
                if not chunk_data:
                    break  # EOF 도달 시 종료
                # 각 청크를 병렬로 처리
                futures.append(executor.submit(process_chunk, chunk_data, chunk_index))

        # 모든 작업 완료 대기
        for future in futures:
            future.result()

    print(f"Finished processing {num_chunks} chunks using {num_threads} threads.")

# 실행 부분
if __name__ == "__main__":
    parse_fasta_multithread(
        fasta_file="/Users/a1/algorithm/Algorithm/random_generator/modified_genome.txt",
        output_prefix="output",
        chunk_size=100,  # 10MB
        num_threads=400          # 4개의 스레드 사용
    )
#nano parse_fasta_multithread.py
#위의 코드 작성
#chmod +x parse_fasta_multithread.py
#python3 parse_fasta_multithread.py
