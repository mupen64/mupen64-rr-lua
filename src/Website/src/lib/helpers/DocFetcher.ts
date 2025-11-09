import * as fs from 'node:fs';
import * as path from 'node:path';


function get_files_in_folder(folder_path: string) {
    try {
        const entries = fs.readdirSync(folder_path, { withFileTypes: true });
        const files = [];

        for (const entry of entries) {
            const full_path = path.join(folder_path, entry.name);
            if (entry.isFile()) {
                files.push(full_path);
            }
        }

        return files;
    } catch (error) {
        console.error('Error reading folder:', error);
        return [];
    }
}
const DOCS_DIR = '../../docs/win';

export async function get_doc_paths() {
    return get_files_in_folder(DOCS_DIR).map(file_path => path.parse(file_path).name);
}