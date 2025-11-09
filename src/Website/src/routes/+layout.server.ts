import type { PageServerLoad } from './$types';
import * as fs from 'node:fs';
import * as path from 'node:path';

const DOCS_DIR = '../../docs/win';

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

export const load: PageServerLoad = async () => {

    let files = get_files_in_folder(DOCS_DIR);
    files = files.map(file_path => path.parse(file_path).name);
    return {
        files
    }
};