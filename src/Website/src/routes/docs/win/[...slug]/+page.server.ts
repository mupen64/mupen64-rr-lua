import matter from 'gray-matter';
import { Marked, marked } from 'marked';
import type { PageServerLoad } from './$types';
import * as fs from 'node:fs';
import * as path from 'node:path';
import { error, redirect } from '@sveltejs/kit';
import { doc_filesystem_to_friendly_name } from '$lib/helpers/DocNameConverter';
import { get_doc_paths } from '$lib/helpers/DocFetcher';

const DOCS_DIR = '../../docs/win';

export const load: PageServerLoad = async ({ params }) => {

    const doc_paths = await get_doc_paths();

    if (params.slug == "") {
        redirect(307, `/docs/win/${doc_paths[0]}`);
    }

    const file_path = path.join(DOCS_DIR, `${params.slug}.md`);
    if (!fs.existsSync(file_path)) {
        error(404, 'Document not found');
    }

    const file = fs.readFileSync(file_path, 'utf-8');
    const { content } = matter(file);
    const marked = new Marked({
        hooks: {
            postprocess(html) {
                html = html.replaceAll('<p', '<p class="y-2"');
                html = html.replaceAll('<h1', '<h1 class="my-4 text-3xl font-bold"');
                html = html.replaceAll('<h2', '<h2 class="my-3 text-2xl"');
                html = html.replaceAll('<h3', '<h3 class="my-2 text-xl"');
                html = html.replaceAll('<code', '<code class="font-mono dark:bg-surface-2 bg-surface-2-light px-1"');
                html = html.replaceAll(
                    '<table',
                    '<table class="my-4 w-full  text-sm rounded-lg overflow-hidden"'
                );
                html = html.replaceAll(
                    '<th',
                    '<th class="bg-surface-2-light dark:bg-surface-2 px-3 py-2 text-left font-semibold"'
                );
                html = html.replaceAll(
                    '<td',
                    '<td class="px-3 py-2 border border-surface-3 dark:border-surface-1"'
                );
                html = html.replaceAll(
                    '<tr',
                    '<tr class="odd:bg-surface-1-light dark:odd:bg-surface-3 even:bg-surface-2-light dark:even:bg-surface-2 transition-colors"'
                );

                return html
            }
        }
    });

    const html = await marked.parse(content);
    console.log(html)

    return {
        content: html,
        title: doc_filesystem_to_friendly_name(params.slug),
    };
};