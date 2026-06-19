import matter from 'gray-matter';
import { doc_name_to_friendly_name } from './DocNameConverter';

export interface DocNode {
    name: string;
    title: string;
    path: string;
    content?: string;
    children?: DocNode[];
    is_dir: boolean;
}

const modules = import.meta.glob('../../../../docs/**/*.md', { as: 'raw', eager: true });

function parseOrderPrefix(name: string): { order: number; cleanName: string } {
    const match = name.match(/^(\d+)\.\s*(.*)$/);
    if (match) {
        return { order: parseInt(match[1], 10), cleanName: match[2] };
    }
    return { order: Number.MAX_SAFE_INTEGER, cleanName: name };
}

function slugifySegment(name: string): string {
    const { cleanName } = parseOrderPrefix(name);
    return cleanName
        .toLowerCase()
        .replace(/[^a-z0-9\s-]/g, '')
        .trim()
        .replace(/\s+/g, '-');
}

function buildTree(paths: Map<string, string>): DocNode {
    const root: DocNode = { name: '', title: '', path: '', is_dir: true, children: [] };

    for (const [fullPath, content] of paths.entries()) {
        // fullPath looks like "../../../../docs/win/1. Getting-Started.md"
        const relative = fullPath.replace(/.*\/?docs\//, '').replace(/\.md$/, '');
        const segments = relative.split('/');
        const parsed = matter(content);

        let current = root;
        let accumulatedPath = '';

        for (let i = 0; i < segments.length; i++) {
            const segment = segments[i];
            const isLast = i === segments.length - 1;
            const slug = slugifySegment(segment);
            accumulatedPath = accumulatedPath ? `${accumulatedPath}/${slug}` : slug;
            const { cleanName } = parseOrderPrefix(segment);
            const defaultTitle = doc_name_to_friendly_name(cleanName);
            const title = isLast && parsed.data.title ? String(parsed.data.title) : defaultTitle;
            const docContent = isLast ? parsed.content : undefined;

            let child = current.children?.find((c) => c.name === segment);
            if (!child) {
                child = {
                    name: segment,
                    title,
                    path: accumulatedPath,
                    is_dir: !isLast,
                    children: !isLast ? [] : undefined,
                    content: docContent,
                };
                current.children!.push(child);
            } else if (isLast) {
                child.content = docContent;
                child.is_dir = false;
                child.title = title;
            }

            current = child;
        }
    }

    sortTree(root);
    return root;
}

function sortTree(node: DocNode) {
    if (!node.children) return;

    node.children.sort((a, b) => {
        // Directories first, then files
        if (a.is_dir !== b.is_dir) {
            return a.is_dir ? -1 : 1;
        }

        const orderA = parseOrderPrefix(a.name).order;
        const orderB = parseOrderPrefix(b.name).order;
        if (orderA !== orderB) {
            return orderA - orderB;
        }

        return a.title.localeCompare(b.title);
    });

    for (const child of node.children) {
        sortTree(child);
    }
}

function loadDocs(): Map<string, string> {
    const result = new Map<string, string>();
    for (const [path, content] of Object.entries(modules)) {
        result.set(path, content as string);
    }
    return result;
}

let tree: DocNode | null = null;

function getTree(): DocNode {
    if (!tree) {
        tree = buildTree(loadDocs());
    }
    return tree;
}

export function get_doc_tree(): DocNode {
    return getTree();
}

export function get_doc_by_path(path: string): DocNode | undefined {
    const root = getTree();
    const segments = path.split('/').filter(Boolean).map(slugifySegment);
    let current: DocNode | undefined = root;

    for (const segment of segments) {
        if (!current?.children) return undefined;
        current = current.children.find((c) => slugifySegment(c.name) === segment);
    }

    return current;
}

export function get_first_doc_path(): string | undefined {
    const root = getTree();
    return findFirstDoc(root);
}

function findFirstDoc(node: DocNode): string | undefined {
    if (!node.is_dir && node.path) {
        return node.path;
    }
    for (const child of node.children || []) {
        const found = findFirstDoc(child);
        if (found) return found;
    }
    return undefined;
}
