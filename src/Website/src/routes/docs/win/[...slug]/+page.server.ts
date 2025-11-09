import matter from 'gray-matter';
import { marked } from 'marked';
import type { PageServerLoad } from './$types';
import * as fs from 'node:fs';
import * as path from 'node:path';
import { error, redirect } from '@sveltejs/kit';
import { doc_filesystem_to_friendly_name } from '$lib/helpers/DocNameConverter';

const DOCS_DIR = '../../docs/win';

export const load: PageServerLoad = async ({ params }) => {

    if (params.slug == "") {
        redirect(307, '/docs/win/home');
    }

    const file_path = path.join(DOCS_DIR, `${params.slug}.md`);
    if (!fs.existsSync(file_path)) {
        error(404, 'Document not found');
    }

    const file = fs.readFileSync(file_path, 'utf-8');
    const { content } = matter(file);
    const html = await marked.parse(content);

    return {
        content: html,
        title: doc_filesystem_to_friendly_name(params.slug),
    };
};