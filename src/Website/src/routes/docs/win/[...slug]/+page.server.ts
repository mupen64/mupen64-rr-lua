import matter from 'gray-matter';
import { marked } from 'marked';
import type { PageServerLoad } from './$types';
import * as fs from 'node:fs';
import * as path from 'node:path';
import type { DocProps } from '$lib/models/DocProps';
import { error, redirect } from '@sveltejs/kit';

const DOCS_DIR = '../../docs/win';

export const load: PageServerLoad = async ({ params }) => {

    if (params.slug == "") {
        redirect(307, '/docs/win/home');
    }

    const filePath = path.join(DOCS_DIR, `${params.slug}.md`);
    if (!fs.existsSync(filePath)) {
        error(404, 'Document not found');
    }

    const file = fs.readFileSync(filePath, 'utf-8');
    const { content } = matter(file);
    const html = await marked.parse(content);

    const props: DocProps = {
        content: html,
    }

    return props;
};